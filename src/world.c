// Copyright 2007-2024 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "lilv_config.h"
#include "lilv_internal.h"

#ifdef LILV_DYN_MANIFEST
#  include "dylib.h"
#  include <lv2/dynmanifest/dynmanifest.h>
#endif

#include <lilv/lilv.h>
#include <lv2/core/lv2.h>
#include <lv2/presets/presets.h>
#include <serd/serd.h>
#include <sord/sord.h>
#include <zix/environment.h>
#include <zix/filesystem.h>
#include <zix/tree.h>

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
lilv_world_drop_graph(LilvWorld* world, const SordNode* graph);

static int
lilv_lib_compare(const void* a, const void* b, const void* user_data);

static void
destroy_node(void* const ptr, const void* const user_data)
{
  (void)user_data;
  lilv_node_free((LilvNode*)ptr);
}

LilvWorld*
lilv_world_new(void)
{
  LilvWorld* world = (LilvWorld*)calloc(1, sizeof(LilvWorld));

  world->world = sord_world_new();
  if (!world->world) {
    goto fail;
  }

  world->model = sord_new(world->world, SORD_SPO | SORD_OPS, true);
  if (!world->model) {
    goto fail;
  }

  world->specs          = NULL;
  world->plugin_classes = lilv_plugin_classes_new();
  world->plugins        = lilv_plugins_new();
  world->zombies        = lilv_plugins_new();

  world->loaded_files =
    zix_tree_new(NULL, false, lilv_resource_node_cmp, NULL, destroy_node, NULL);

  world->libs = zix_tree_new(NULL, false, lilv_lib_compare, NULL, NULL, NULL);

#define NS_DCTERMS "http://purl.org/dc/terms/"
#define NS_DYNMAN "http://lv2plug.in/ns/ext/dynmanifest#"
#define NS_OWL "http://www.w3.org/2002/07/owl#"

#define NEW_URI(uri) sord_new_uri(world->world, (const uint8_t*)(uri))

  world->uris.dc_replaces         = NEW_URI(NS_DCTERMS "replaces");
  world->uris.dman_DynManifest    = NEW_URI(NS_DYNMAN "DynManifest");
  world->uris.doap_name           = NEW_URI(LILV_NS_DOAP "name");
  world->uris.lv2_Plugin          = NEW_URI(LV2_CORE__Plugin);
  world->uris.lv2_Specification   = NEW_URI(LV2_CORE__Specification);
  world->uris.lv2_appliesTo       = NEW_URI(LV2_CORE__appliesTo);
  world->uris.lv2_binary          = NEW_URI(LV2_CORE__binary);
  world->uris.lv2_default         = NEW_URI(LV2_CORE__default);
  world->uris.lv2_designation     = NEW_URI(LV2_CORE__designation);
  world->uris.lv2_extensionData   = NEW_URI(LV2_CORE__extensionData);
  world->uris.lv2_index           = NEW_URI(LV2_CORE__index);
  world->uris.lv2_latency         = NEW_URI(LV2_CORE__latency);
  world->uris.lv2_maximum         = NEW_URI(LV2_CORE__maximum);
  world->uris.lv2_microVersion    = NEW_URI(LV2_CORE__microVersion);
  world->uris.lv2_minimum         = NEW_URI(LV2_CORE__minimum);
  world->uris.lv2_minorVersion    = NEW_URI(LV2_CORE__minorVersion);
  world->uris.lv2_name            = NEW_URI(LV2_CORE__name);
  world->uris.lv2_optionalFeature = NEW_URI(LV2_CORE__optionalFeature);
  world->uris.lv2_port            = NEW_URI(LV2_CORE__port);
  world->uris.lv2_portProperty    = NEW_URI(LV2_CORE__portProperty);
  world->uris.lv2_reportsLatency  = NEW_URI(LV2_CORE__reportsLatency);
  world->uris.lv2_requiredFeature = NEW_URI(LV2_CORE__requiredFeature);
  world->uris.lv2_symbol          = NEW_URI(LV2_CORE__symbol);
  world->uris.lv2_prototype       = NEW_URI(LV2_CORE__prototype);
  world->uris.owl_Ontology        = NEW_URI(NS_OWL "Ontology");
  world->uris.pset_value          = NEW_URI(LV2_PRESETS__value);
  world->uris.rdf_a               = NEW_URI(LILV_NS_RDF "type");
  world->uris.rdf_value           = NEW_URI(LILV_NS_RDF "value");
  world->uris.rdfs_Class          = NEW_URI(LILV_NS_RDFS "Class");
  world->uris.rdfs_label          = NEW_URI(LILV_NS_RDFS "label");
  world->uris.rdfs_seeAlso        = NEW_URI(LILV_NS_RDFS "seeAlso");
  world->uris.rdfs_subClassOf     = NEW_URI(LILV_NS_RDFS "subClassOf");
  world->uris.xsd_base64Binary    = NEW_URI(LILV_NS_XSD "base64Binary");
  world->uris.xsd_boolean         = NEW_URI(LILV_NS_XSD "boolean");
  world->uris.xsd_decimal         = NEW_URI(LILV_NS_XSD "decimal");
  world->uris.xsd_double          = NEW_URI(LILV_NS_XSD "double");
  world->uris.xsd_integer         = NEW_URI(LILV_NS_XSD "integer");
  world->uris.null_uri            = NULL;

  world->lv2_plugin_class =
    lilv_plugin_class_new(world, NULL, world->uris.lv2_Plugin, "Plugin");
  assert(world->lv2_plugin_class);

  world->n_read_files        = 0;
  world->opt.filter_language = true;
  world->opt.dyn_manifest    = true;

  return world;

fail:
  /* keep on rockin' in the */ free(world);
  return NULL;
}

void
lilv_world_free(LilvWorld* world)
{
  if (!world) {
    return;
  }

  lilv_plugin_class_free(world->lv2_plugin_class);
  world->lv2_plugin_class = NULL;

  for (SordNode** n = (SordNode**)&world->uris; *n; ++n) {
    sord_node_free(world->world, *n);
  }

  for (LilvSpec* spec = world->specs; spec;) {
    LilvSpec* next = spec->next;
    sord_node_free(world->world, spec->spec);
    sord_node_free(world->world, spec->bundle);
    lilv_nodes_free(spec->data_uris);
    free(spec);
    spec = next;
  }
  world->specs = NULL;

  LILV_FOREACH (plugins, i, world->plugins) {
    const LilvPlugin* p = lilv_plugins_get(world->plugins, i);
    lilv_plugin_free((LilvPlugin*)p);
  }
  zix_tree_free((ZixTree*)world->plugins);
  world->plugins = NULL;

  LILV_FOREACH (plugins, i, world->zombies) {
    const LilvPlugin* p = lilv_plugins_get(world->zombies, i);
    lilv_plugin_free((LilvPlugin*)p);
  }
  zix_tree_free((ZixTree*)world->zombies);
  world->zombies = NULL;

  zix_tree_free((ZixTree*)world->loaded_files);
  world->loaded_files = NULL;

  zix_tree_free(world->libs);
  world->libs = NULL;

  zix_tree_free((ZixTree*)world->plugin_classes);
  world->plugin_classes = NULL;

  sord_free(world->model);
  world->model = NULL;

  sord_world_free(world->world);
  world->world = NULL;

  free(world->opt.lv2_path);
  free(world);
}

void
lilv_world_set_option(LilvWorld* world, const char* uri, const LilvNode* value)
{
  if (!strcmp(uri, LILV_OPTION_DYN_MANIFEST)) {
    if (lilv_node_is_bool(value)) {
      world->opt.dyn_manifest = lilv_node_as_bool(value);
      return;
    }
  } else if (!strcmp(uri, LILV_OPTION_FILTER_LANG)) {
    if (lilv_node_is_bool(value)) {
      world->opt.filter_language = lilv_node_as_bool(value);
      return;
    }
  } else if (!strcmp(uri, LILV_OPTION_LV2_PATH)) {
    if (lilv_node_is_string(value)) {
      world->opt.lv2_path = lilv_strdup(lilv_node_as_string(value));
      return;
    }
  }
  LILV_WARNF("Unrecognized or invalid option `%s'\n", uri);
}

LilvNodes*
lilv_world_find_nodes(LilvWorld*      world,
                      const LilvNode* subject,
                      const LilvNode* predicate,
                      const LilvNode* object)
{
  if (subject && !lilv_node_is_uri(subject) && !lilv_node_is_blank(subject)) {
    LILV_ERRORF("Subject `%s' is not a resource\n",
                sord_node_get_string(subject->node));
    return NULL;
  }

  if (!predicate) {
    LILV_ERROR("Missing required predicate\n");
    return NULL;
  }

  if (!lilv_node_is_uri(predicate)) {
    LILV_ERRORF("Predicate `%s' is not a URI\n",
                sord_node_get_string(predicate->node));
    return NULL;
  }

  if (!subject && !object) {
    LILV_ERROR("Both subject and object are NULL\n");
    return NULL;
  }

  return lilv_world_find_nodes_internal(world,
                                        subject ? subject->node : NULL,
                                        predicate->node,
                                        object ? object->node : NULL);
}

LilvNode*
lilv_world_get(LilvWorld*      world,
               const LilvNode* subject,
               const LilvNode* predicate,
               const LilvNode* object)
{
  if (!object) {
    // TODO: Improve performance (see lilv_plugin_get_one)
    SordIter* stream = sord_search(world->model,
                                   subject ? subject->node : NULL,
                                   predicate ? predicate->node : NULL,
                                   NULL,
                                   NULL);

    LilvNodes* nodes =
      lilv_nodes_from_stream_objects(world, stream, SORD_OBJECT);

    if (nodes) {
      LilvNode* value = lilv_node_duplicate(lilv_nodes_get_first(nodes));
      lilv_nodes_free(nodes);
      return value;
    }

    return NULL;
  }

  SordNode* snode = sord_get(world->model,
                             subject ? subject->node : NULL,
                             predicate ? predicate->node : NULL,
                             object->node,
                             NULL);
  LilvNode* lnode = lilv_node_new_from_node(world, snode);
  sord_node_free(world->world, snode);
  return lnode;
}

SordIter*
lilv_world_query_internal(LilvWorld*      world,
                          const SordNode* subject,
                          const SordNode* predicate,
                          const SordNode* object)
{
  return sord_search(world->model, subject, predicate, object, NULL);
}

bool
lilv_world_ask_internal(LilvWorld*      world,
                        const SordNode* subject,
                        const SordNode* predicate,
                        const SordNode* object)
{
  return sord_ask(world->model, subject, predicate, object, NULL);
}

bool
lilv_world_ask(LilvWorld*      world,
               const LilvNode* subject,
               const LilvNode* predicate,
               const LilvNode* object)
{
  return sord_ask(world->model,
                  subject ? subject->node : NULL,
                  predicate ? predicate->node : NULL,
                  object ? object->node : NULL,
                  NULL);
}

SordModel*
lilv_world_filter_model(LilvWorld*      world,
                        SordModel*      model,
                        const SordNode* subject,
                        const SordNode* predicate,
                        const SordNode* object,
                        const SordNode* graph)
{
  SordModel* results = sord_new(world->world, SORD_SPO, false);
  SordIter*  i       = sord_search(model, subject, predicate, object, graph);
  for (; !sord_iter_end(i); sord_iter_next(i)) {
    SordQuad quad;
    sord_iter_get(i, quad);
    sord_add(results, quad);
  }
  sord_iter_free(i);
  return results;
}

LilvNodes*
lilv_world_find_nodes_internal(LilvWorld*      world,
                               const SordNode* subject,
                               const SordNode* predicate,
                               const SordNode* object)
{
  return lilv_nodes_from_stream_objects(
    world,
    lilv_world_query_internal(world, subject, predicate, object),
    (object == NULL) ? SORD_OBJECT : SORD_SUBJECT);
}

const uint8_t*
lilv_world_blank_node_prefix(LilvWorld* world)
{
  static char str[32];
  snprintf(str, sizeof(str), "%u", world->n_read_files++);
  return (const uint8_t*)str;
}

/** Comparator for sequences (e.g. world->plugins). */
int
lilv_header_compare_by_uri(const void* a, const void* b, const void* user_data)
{
  (void)user_data;

  const struct LilvHeader* const header_a = (const struct LilvHeader*)a;
  const struct LilvHeader* const header_b = (const struct LilvHeader*)b;

  return strcmp(lilv_node_as_uri(header_a->uri),
                lilv_node_as_uri(header_b->uri));
}

/**
   Comparator for libraries (world->libs).

   Libraries do have a LilvHeader, but we must also compare the bundle to
   handle the case where the same library is loaded with different bundles, and
   consequently different contents (mainly plugins).
 */
static int
lilv_lib_compare(const void* a, const void* b, const void* user_data)
{
  (void)user_data;

  const LilvLib* const lib_a = (const LilvLib*)a;
  const LilvLib* const lib_b = (const LilvLib*)b;

  const int cmp =
    strcmp(lilv_node_as_uri(lib_a->uri), lilv_node_as_uri(lib_b->uri));

  return cmp ? cmp : strcmp(lib_a->bundle_path, lib_b->bundle_path);
}

/** Get an element of a collection of any object with an LilvHeader by URI. */
static ZixTreeIter*
lilv_collection_find_by_uri(const ZixTree* seq, const LilvNode* uri)
{
  ZixTreeIter* i = NULL;
  if (lilv_node_is_uri(uri)) {
    struct LilvHeader key = {NULL, (LilvNode*)uri};
    (void)zix_tree_find(seq, &key, &i);
  }
  return i;
}

/** Get an element of a collection of any object with an LilvHeader by URI. */
struct LilvHeader*
lilv_collection_get_by_uri(const ZixTree* seq, const LilvNode* uri)
{
  const ZixTreeIter* const i = lilv_collection_find_by_uri(seq, uri);

  return i ? (struct LilvHeader*)zix_tree_get(i) : NULL;
}

static void
lilv_world_add_spec(LilvWorld*      world,
                    const SordNode* specification_node,
                    const SordNode* bundle_node)
{
  LilvSpec* spec  = (LilvSpec*)malloc(sizeof(LilvSpec));
  spec->spec      = sord_node_copy(specification_node);
  spec->bundle    = sord_node_copy(bundle_node);
  spec->data_uris = lilv_nodes_new();

  // Add all data files (rdfs:seeAlso)
  SordIter* files = sord_search(
    world->model, specification_node, world->uris.rdfs_seeAlso, NULL, NULL);
  FOREACH_MATCH (files) {
    const SordNode* file_node = sord_iter_get_node(files, SORD_OBJECT);
    zix_tree_insert((ZixTree*)spec->data_uris,
                    lilv_node_new_from_node(world, file_node),
                    NULL);
  }
  sord_iter_free(files);

  // Add specification to world specification list
  spec->next   = world->specs;
  world->specs = spec;
}

static void
lilv_world_add_plugin(LilvWorld*      world,
                      const SordNode* plugin_node,
                      const LilvNode* manifest_uri,
                      void*           dynmanifest,
                      const SordNode* bundle)
{
  (void)dynmanifest;

  LilvNode*    plugin_uri = lilv_node_new_from_node(world, plugin_node);
  ZixTreeIter* z          = NULL;
  LilvPlugin*  plugin =
    (LilvPlugin*)lilv_plugins_get_by_uri(world->plugins, plugin_uri);

  if (plugin) {
    // Existing plugin, if this is different bundle, ignore it
    // (use the first plugin found in LV2_PATH)
    const LilvNode* last_bundle    = lilv_plugin_get_bundle_uri(plugin);
    const char*     plugin_uri_str = lilv_node_as_uri(plugin_uri);
    if (sord_node_equals(bundle, last_bundle->node)) {
      LILV_WARNF("Reloading plugin <%s>\n", plugin_uri_str);
      plugin->loaded = false;
      lilv_node_free(plugin_uri);
    } else {
      LILV_WARNF("Duplicate plugin <%s>\n", plugin_uri_str);
      LILV_WARNF("... found in %s\n", lilv_node_as_string(last_bundle));
      LILV_WARNF("... and      %s (ignored)\n", sord_node_get_string(bundle));
      lilv_node_free(plugin_uri);
      return;
    }
  } else if ((z = lilv_collection_find_by_uri((const ZixTree*)world->zombies,
                                              plugin_uri))) {
    // Plugin bundle has been re-loaded, move from zombies to plugins
    plugin = (LilvPlugin*)zix_tree_get(z);
    zix_tree_remove((ZixTree*)world->zombies, z);
    zix_tree_insert((ZixTree*)world->plugins, plugin, NULL);
    lilv_node_free(plugin_uri);
    lilv_plugin_clear(plugin, lilv_node_new_from_node(world, bundle));
  } else {
    // Add new plugin to the world
    plugin = lilv_plugin_new(
      world, plugin_uri, lilv_node_new_from_node(world, bundle));

    // Add manifest as plugin data file (as if it were rdfs:seeAlso)
    zix_tree_insert(
      (ZixTree*)plugin->data_uris, lilv_node_duplicate(manifest_uri), NULL);

    // Add plugin to world plugin sequence
    zix_tree_insert((ZixTree*)world->plugins, plugin, NULL);
  }

#ifdef LILV_DYN_MANIFEST
  // Set dynamic manifest library URI, if applicable
  if (dynmanifest) {
    plugin->dynmanifest = (LilvDynManifest*)dynmanifest;
    ++((LilvDynManifest*)dynmanifest)->refs;
  }
#endif

  // Add all plugin data files (rdfs:seeAlso)
  SordIter* files = sord_search(
    world->model, plugin_node, world->uris.rdfs_seeAlso, NULL, NULL);
  FOREACH_MATCH (files) {
    const SordNode* file_node = sord_iter_get_node(files, SORD_OBJECT);
    zix_tree_insert((ZixTree*)plugin->data_uris,
                    lilv_node_new_from_node(world, file_node),
                    NULL);
  }
  sord_iter_free(files);
}

static SerdStatus
lilv_world_load_graph(LilvWorld* world, SordNode* graph, const LilvNode* uri)
{
  const SerdNode* base = sord_node_to_serd_node(uri->node);
  SerdEnv*        env  = serd_env_new(base);
  SerdReader* reader   = sord_new_reader(world->model, env, SERD_TURTLE, graph);

  const SerdStatus st = lilv_world_load_file(world, reader, uri);

  serd_env_free(env);
  serd_reader_free(reader);
  return st;
}

static void
lilv_world_load_dyn_manifest(LilvWorld*      world,
                             SordNode*       bundle_node,
                             const LilvNode* manifest)
{
#ifdef LILV_DYN_MANIFEST
  if (!world->opt.dyn_manifest) {
    return;
  }

  LV2_Dyn_Manifest_Handle handle = NULL;

  // ?dman a dynman:DynManifest bundle_node
  SordModel* model = lilv_world_filter_model(world,
                                             world->model,
                                             NULL,
                                             world->uris.rdf_a,
                                             world->uris.dman_DynManifest,
                                             bundle_node);
  SordIter*  iter  = sord_begin(model);
  for (; !sord_iter_end(iter); sord_iter_next(iter)) {
    const SordNode* dmanifest = sord_iter_get_node(iter, SORD_SUBJECT);

    // ?dman lv2:binary ?binary
    SordIter* binaries = sord_search(
      world->model, dmanifest, world->uris.lv2_binary, NULL, bundle_node);
    if (sord_iter_end(binaries)) {
      sord_iter_free(binaries);
      LILV_ERRORF("Dynamic manifest in <%s> has no binaries, ignored\n",
                  sord_node_get_string(bundle_node));
      continue;
    }

    // Get binary path
    const SordNode* binary   = sord_iter_get_node(binaries, SORD_OBJECT);
    const uint8_t*  lib_uri  = sord_node_get_string(binary);
    char*           lib_path = lilv_file_uri_parse((const char*)lib_uri, 0);
    if (!lib_path) {
      LILV_ERROR("No dynamic manifest library path\n");
      sord_iter_free(binaries);
      continue;
    }

    // Open library
    dylib_error();
    void* lib = dylib_open(lib_path, DYLIB_LAZY);
    if (!lib) {
      LILV_ERRORF(
        "Failed to open dynmanifest library `%s' (%s)\n", lib_path, dlerror());
      sord_iter_free(binaries);
      lilv_free(lib_path);
      continue;
    }

    // Open dynamic manifest
    typedef int (*OpenFunc)(LV2_Dyn_Manifest_Handle*,
                            const LV2_Feature* const*);
    OpenFunc dmopen = (OpenFunc)dylib_func(lib, "lv2_dyn_manifest_open");
    if (!dmopen || dmopen(&handle, &dman_features)) {
      LILV_ERRORF("No `lv2_dyn_manifest_open' in `%s'\n", lib_path);
      sord_iter_free(binaries);
      dylib_close(lib);
      lilv_free(lib_path);
      continue;
    }

    // Get subjects (the data that would be in manifest.ttl)
    typedef int (*GetSubjectsFunc)(LV2_Dyn_Manifest_Handle, FILE*);
    GetSubjectsFunc get_subjects_func =
      (GetSubjectsFunc)dylib_func(lib, "lv2_dyn_manifest_get_subjects");
    if (!get_subjects_func) {
      LILV_ERRORF("No `lv2_dyn_manifest_get_subjects' in `%s'\n", lib_path);
      sord_iter_free(binaries);
      dylib_close(lib);
      lilv_free(lib_path);
      continue;
    }

    LilvDynManifest* desc = (LilvDynManifest*)malloc(sizeof(LilvDynManifest));
    desc->bundle          = lilv_node_new_from_node(world, bundle_node);
    desc->lib             = lib;
    desc->handle          = handle;
    desc->refs            = 0;

    sord_iter_free(binaries);

    // Generate data file
    FILE* fd = tmpfile();
    get_subjects_func(handle, fd);
    rewind(fd);

    // Parse generated data file into temporary model
    // FIXME
    const SerdNode* base   = sord_node_to_serd_node(dmanifest);
    SerdEnv*        env    = serd_env_new(base);
    SerdReader*     reader = sord_new_reader(
      world->model, env, SERD_TURTLE, sord_node_copy(dmanifest));
    serd_reader_add_blank_prefix(reader, lilv_world_blank_node_prefix(world));
    serd_reader_read_file_handle(reader, fd, (const uint8_t*)"(dyn-manifest)");
    serd_reader_free(reader);
    serd_env_free(env);

    // Close (and automatically delete) temporary data file
    fclose(fd);

    // ?plugin a lv2:Plugin
    SordModel* plugins = lilv_world_filter_model(world,
                                                 world->model,
                                                 NULL,
                                                 world->uris.rdf_a,
                                                 world->uris.lv2_Plugin,
                                                 dmanifest);
    SordIter*  p       = sord_begin(plugins);
    FOREACH_MATCH (p) {
      const SordNode* plug = sord_iter_get_node(p, SORD_SUBJECT);
      lilv_world_add_plugin(world, plug, manifest, desc, bundle_node);
    }
    if (desc->refs == 0) {
      lilv_dynmanifest_free(desc);
    }
    sord_iter_free(p);
    sord_free(plugins);
    lilv_free(lib_path);
  }
  sord_iter_free(iter);
  sord_free(model);

#else // LILV_DYN_MANIFEST
  (void)world;
  (void)bundle_node;
  (void)manifest;
#endif
}

#ifdef LILV_DYN_MANIFEST
void
lilv_dynmanifest_free(LilvDynManifest* dynmanifest)
{
  typedef int (*CloseFunc)(LV2_Dyn_Manifest_Handle);
  CloseFunc close_func =
    (CloseFunc)dylib_func(dynmanifest->lib, "lv2_dyn_manifest_close");
  if (close_func) {
    close_func(dynmanifest->handle);
  }

  dylib_close(dynmanifest->lib);
  lilv_node_free(dynmanifest->bundle);
  free(dynmanifest);
}
#endif // LILV_DYN_MANIFEST

LilvNode*
lilv_world_get_manifest_uri(LilvWorld* world, const LilvNode* bundle_uri)
{
  // Get the string and length of the given bundle URI
  size_t            bundle_uri_length = 0U;
  const char* const bundle_uri_string =
    (const char*)sord_node_get_string_counted(bundle_uri->node,
                                              &bundle_uri_length);
  if (bundle_uri_length < 1U) {
    return NULL;
  }

  // Build the manifest URI by inserting a separating "/" if necessary
  const char  last = bundle_uri_string[bundle_uri_length - 1U];
  char* const manifest_uri_string =
    (last == '/') ? lilv_strjoin(bundle_uri_string, "manifest.ttl", NULL)
                  : lilv_strjoin(bundle_uri_string, "/", "manifest.ttl", NULL);

  // Make a node from the manifeset URI to return
  LilvNode* const manifest = lilv_new_uri(world, manifest_uri_string);
  free(manifest_uri_string);
  return manifest;
}

static SordModel*
load_plugin_model(LilvWorld*      world,
                  const LilvNode* bundle_uri,
                  const LilvNode* plugin_uri)
{
  // Create model and reader for loading into it
  const SordNode* bundle_node = bundle_uri->node;
  SordModel*      model  = sord_new(world->world, SORD_SPO | SORD_OPS, false);
  SerdEnv*        env    = serd_env_new(sord_node_to_serd_node(bundle_node));
  SerdReader*     reader = sord_new_reader(model, env, SERD_TURTLE, NULL);

  // Load manifest
  LilvNode* manifest_uri = lilv_world_get_manifest_uri(world, bundle_uri);
  serd_reader_add_blank_prefix(reader, lilv_world_blank_node_prefix(world));
  serd_reader_read_file(reader,
                        (const uint8_t*)lilv_node_as_string(manifest_uri));

  // Load any seeAlso files
  SordModel* files = lilv_world_filter_model(
    world, model, plugin_uri->node, world->uris.rdfs_seeAlso, NULL, NULL);

  SordIter* f = sord_begin(files);
  FOREACH_MATCH (f) {
    const SordNode* file     = sord_iter_get_node(f, SORD_OBJECT);
    const uint8_t*  file_str = sord_node_get_string(file);
    if (sord_node_get_type(file) == SORD_URI) {
      serd_reader_add_blank_prefix(reader, lilv_world_blank_node_prefix(world));
      serd_reader_read_file(reader, file_str);
    }
  }

  sord_iter_free(f);
  sord_free(files);
  serd_reader_free(reader);
  serd_env_free(env);
  lilv_node_free(manifest_uri);

  return model;
}

static LilvVersion
get_version(const LilvWorld* world, SordModel* model, const LilvNode* subject)
{
  const SordNode* minor_node =
    sord_get(model, subject->node, world->uris.lv2_minorVersion, NULL, NULL);
  const SordNode* micro_node =
    sord_get(model, subject->node, world->uris.lv2_microVersion, NULL, NULL);

  LilvVersion version = {0, 0};
  if (minor_node && micro_node) {
    const char* const minor_str = (const char*)sord_node_get_string(minor_node);
    const char* const micro_str = (const char*)sord_node_get_string(micro_node);
    const long        minor     = strtol(minor_str, NULL, 10);
    const long        micro     = strtol(micro_str, NULL, 10);
    if (minor >= 0 && minor < INT_MAX && micro >= 0 && micro < INT_MAX) {
      version.minor = (int)minor;
      version.micro = (int)micro;
    }
  }

  return version;
}

void
lilv_world_load_bundle(LilvWorld* world, const LilvNode* bundle_uri)
{
  if (!lilv_node_is_uri(bundle_uri)) {
    LILV_ERRORF("Bundle URI `%s' is not a URI\n",
                sord_node_get_string(bundle_uri->node));
    return;
  }

  SordNode* bundle_node = bundle_uri->node;
  LilvNode* manifest    = lilv_world_get_manifest_uri(world, bundle_uri);
  if (!manifest) {
    return;
  }

  // Read manifest into model with graph = bundle_node
  SerdStatus st = lilv_world_load_graph(world, bundle_node, manifest);
  if (st > SERD_FAILURE) {
    LILV_ERRORF("Error reading %s\n", lilv_node_as_string(manifest));
    lilv_node_free(manifest);
    return;
  }

  // ?plugin a lv2:Plugin
  SordIter* plug_results = sord_search(
    world->model, NULL, world->uris.rdf_a, world->uris.lv2_Plugin, bundle_node);

  // Find any loaded plugins that will be replaced with a newer version
  LilvNodes* unload_uris = lilv_nodes_new();
  FOREACH_MATCH (plug_results) {
    const SordNode* plug = sord_iter_get_node(plug_results, SORD_SUBJECT);

    LilvNode*         plugin_uri = lilv_node_new_from_node(world, plug);
    const LilvPlugin* plugin =
      lilv_plugins_get_by_uri(world->plugins, plugin_uri);
    const LilvNode* last_bundle =
      plugin ? lilv_plugin_get_bundle_uri(plugin) : NULL;
    if (!plugin || sord_node_equals(bundle_node, last_bundle->node)) {
      // No previously loaded version, or it's from the same bundle
      lilv_node_free(plugin_uri);
      continue;
    }

    // Compare versions
    SordModel*  this_model   = load_plugin_model(world, bundle_uri, plugin_uri);
    LilvVersion this_version = get_version(world, this_model, plugin_uri);
    SordModel*  last_model = load_plugin_model(world, last_bundle, plugin_uri);
    LilvVersion last_version = get_version(world, last_model, plugin_uri);
    sord_free(this_model);
    sord_free(last_model);
    const int cmp = lilv_version_cmp(&this_version, &last_version);
    if (cmp > 0) {
      zix_tree_insert(
        (ZixTree*)unload_uris, lilv_node_duplicate(plugin_uri), NULL);
      LILV_WARNF("Replacing version %d.%d of <%s> from <%s>\n",
                 last_version.minor,
                 last_version.micro,
                 sord_node_get_string(plug),
                 sord_node_get_string(last_bundle->node));
      LILV_NOTEF("New version %d.%d found in <%s>\n",
                 this_version.minor,
                 this_version.micro,
                 sord_node_get_string(bundle_node));
    } else if (cmp < 0) {
      LILV_WARNF("Ignoring bundle <%s>\n", sord_node_get_string(bundle_node));
      LILV_NOTEF("Newer version of <%s> loaded from <%s>\n",
                 sord_node_get_string(plug),
                 sord_node_get_string(last_bundle->node));
      lilv_node_free(plugin_uri);
      sord_iter_free(plug_results);
      lilv_world_drop_graph(world, bundle_node);
      lilv_node_free(manifest);
      lilv_nodes_free(unload_uris);
      return;
    }
    lilv_node_free(plugin_uri);
  }

  sord_iter_free(plug_results);

  // Unload any old conflicting plugins
  LilvNodes* unload_bundles = lilv_nodes_new();
  LILV_FOREACH (nodes, i, unload_uris) {
    const LilvNode*   uri    = lilv_nodes_get(unload_uris, i);
    const LilvPlugin* plugin = lilv_plugins_get_by_uri(world->plugins, uri);
    const LilvNode*   bundle = lilv_plugin_get_bundle_uri(plugin);

    // Unload plugin and record bundle for later unloading
    lilv_world_unload_resource(world, uri);
    zix_tree_insert(
      (ZixTree*)unload_bundles, lilv_node_duplicate(bundle), NULL);
  }
  lilv_nodes_free(unload_uris);

  // Now unload the associated bundles
  // This must be done last since several plugins could be in the same bundle
  LILV_FOREACH (nodes, i, unload_bundles) {
    lilv_world_unload_bundle(world, lilv_nodes_get(unload_bundles, i));
  }
  lilv_nodes_free(unload_bundles);

  // Re-search for plugin results now that old plugins are gone
  plug_results = sord_search(
    world->model, NULL, world->uris.rdf_a, world->uris.lv2_Plugin, bundle_node);

  FOREACH_MATCH (plug_results) {
    const SordNode* plug = sord_iter_get_node(plug_results, SORD_SUBJECT);
    lilv_world_add_plugin(world, plug, manifest, NULL, bundle_node);
  }
  sord_iter_free(plug_results);

  lilv_world_load_dyn_manifest(world, bundle_node, manifest);

  // ?spec a lv2:Specification
  // ?spec a owl:Ontology
  const SordNode* spec_preds[] = {
    world->uris.lv2_Specification, world->uris.owl_Ontology, NULL};
  for (const SordNode** p = spec_preds; *p; ++p) {
    SordIter* i =
      sord_search(world->model, NULL, world->uris.rdf_a, *p, bundle_node);
    FOREACH_MATCH (i) {
      const SordNode* spec = sord_iter_get_node(i, SORD_SUBJECT);
      lilv_world_add_spec(world, spec, bundle_node);
    }
    sord_iter_free(i);
  }

  lilv_node_free(manifest);
}

static int
lilv_world_drop_graph(LilvWorld* world, const SordNode* graph)
{
  SordIter* i = sord_search(world->model, NULL, NULL, NULL, graph);
  while (!sord_iter_end(i)) {
    const SerdStatus st = sord_erase(world->model, i);
    if (st) {
      sord_iter_free(i);
      LILV_ERRORF("Error removing statement from <%s> (%s)\n",
                  sord_node_get_string(graph),
                  serd_strerror(st));
      return st;
    }
  }
  sord_iter_free(i);

  return 0;
}

/** Remove loaded_files entry so file will be reloaded if requested. */
static int
lilv_world_unload_file(LilvWorld* world, const LilvNode* file)
{
  ZixTreeIter* iter = NULL;
  if (!zix_tree_find((ZixTree*)world->loaded_files, file, &iter)) {
    zix_tree_remove((ZixTree*)world->loaded_files, iter);
    return 0;
  }
  return 1;
}

int
lilv_world_unload_bundle(LilvWorld* world, const LilvNode* bundle_uri)
{
  if (!bundle_uri) {
    return 0;
  }

  // Find all loaded files that are inside the bundle
  LilvNodes* files = lilv_nodes_new();
  LILV_FOREACH (nodes, i, world->loaded_files) {
    const LilvNode* file = lilv_nodes_get(world->loaded_files, i);
    if (!strncmp(lilv_node_as_string(file),
                 lilv_node_as_string(bundle_uri),
                 strlen(lilv_node_as_string(bundle_uri)))) {
      zix_tree_insert((ZixTree*)files, lilv_node_duplicate(file), NULL);
    }
  }

  // Unload all loaded files in the bundle
  LILV_FOREACH (nodes, i, files) {
    const LilvNode* file = lilv_nodes_get(world->plugins, i);
    lilv_world_unload_file(world, file);
  }

  lilv_nodes_free(files);

  /* Remove any plugins in the bundle from the plugin list.  Since the
     application may still have a pointer to the LilvPlugin, it can not be
     destroyed here.  Instead, we move it to the zombie plugin list, so it
     will not be in the list returned by lilv_world_get_all_plugins() but can
     still be used.
  */
  ZixTreeIter* i = zix_tree_begin((ZixTree*)world->plugins);
  while (i && i != zix_tree_end((ZixTree*)world->plugins)) {
    LilvPlugin*  p    = (LilvPlugin*)zix_tree_get(i);
    ZixTreeIter* next = zix_tree_iter_next(i);

    if (lilv_node_equals(lilv_plugin_get_bundle_uri(p), bundle_uri)) {
      zix_tree_remove((ZixTree*)world->plugins, i);
      zix_tree_insert((ZixTree*)world->zombies, p, NULL);
    }

    i = next;
  }

  // Drop everything in bundle graph
  return lilv_world_drop_graph(world, bundle_uri->node);
}

static void
load_dir_entry(const char* dir, const char* name, void* data)
{
  LilvWorld* world = (LilvWorld*)data;
  char*      path  = lilv_strjoin(dir, "/", name, "/", NULL);
  SerdNode   suri  = serd_node_new_file_uri((const uint8_t*)path, 0, 0, true);
  LilvNode*  node  = lilv_new_uri(world, (const char*)suri.buf);

  lilv_world_load_bundle(world, node);
  lilv_node_free(node);
  serd_node_free(&suri);
  free(path);
}

/** Load all bundles in the directory at `dir_path`. */
static void
lilv_world_load_directory(LilvWorld* world, const char* dir_path)
{
  char* const path = zix_expand_environment_strings(NULL, dir_path);
  if (path) {
    zix_dir_for_each(path, world, load_dir_entry);
    free(path);
  }
}

static const char*
first_path_sep(const char* path)
{
  for (const char* p = path; *p != '\0'; ++p) {
    if (*p == LILV_PATH_SEP[0]) {
      return p;
    }
  }
  return NULL;
}

/** Load all bundles found in `lv2_path`.
 * @param lv2_path A colon-delimited list of directories.  These directories
 * should contain LV2 bundle directories (ie the search path is a list of
 * parent directories of bundles, not a list of bundle directories).
 */
static void
lilv_world_load_path(LilvWorld* world, const char* lv2_path)
{
  while (lv2_path[0] != '\0') {
    const char* const sep = first_path_sep(lv2_path);
    if (sep) {
      const size_t dir_len = sep - lv2_path;
      char* const  dir     = (char*)malloc(dir_len + 1);
      memcpy(dir, lv2_path, dir_len);
      dir[dir_len] = '\0';
      lilv_world_load_directory(world, dir);
      free(dir);
      lv2_path += dir_len + 1;
    } else {
      lilv_world_load_directory(world, lv2_path);
      lv2_path = "\0";
    }
  }
}

void
lilv_world_load_specifications(LilvWorld* world)
{
  for (LilvSpec* spec = world->specs; spec; spec = spec->next) {
    LILV_FOREACH (nodes, f, spec->data_uris) {
      const LilvNode* file =
        (const LilvNode*)lilv_collection_get(spec->data_uris, f);
      lilv_world_load_graph(world, NULL, file);
    }
  }
}

void
lilv_world_load_plugin_classes(LilvWorld* world)
{
  /* FIXME: This loads all classes, not just lv2:Plugin subclasses.
     However, if the host gets all the classes via
     lilv_plugin_class_get_children starting with lv2:Plugin as the root (which
     is e.g. how a host would build a menu), they won't be seen anyway...
  */

  SordIter* classes = sord_search(
    world->model, NULL, world->uris.rdf_a, world->uris.rdfs_Class, NULL);
  FOREACH_MATCH (classes) {
    const SordNode* class_node = sord_iter_get_node(classes, SORD_SUBJECT);

    SordNode* parent = sord_get(
      world->model, class_node, world->uris.rdfs_subClassOf, NULL, NULL);
    if (!parent || sord_node_get_type(parent) != SORD_URI) {
      continue;
    }

    SordNode* label =
      sord_get(world->model, class_node, world->uris.rdfs_label, NULL, NULL);
    if (!label) {
      sord_node_free(world->world, parent);
      continue;
    }

    LilvPluginClass* pclass = lilv_plugin_class_new(
      world, parent, class_node, (const char*)sord_node_get_string(label));
    if (pclass) {
      zix_tree_insert((ZixTree*)world->plugin_classes, pclass, NULL);
    }

    sord_node_free(world->world, label);
    sord_node_free(world->world, parent);
  }
  sord_iter_free(classes);
}

void
lilv_world_load_all(LilvWorld* world)
{
  const char* lv2_path = world->opt.lv2_path;
  if (!lv2_path) {
    lv2_path = getenv("LV2_PATH");
  }
  if (!lv2_path) {
    lv2_path = LILV_DEFAULT_LV2_PATH;
  }

  // Discover bundles and read all manifest files into model
  lilv_world_load_path(world, lv2_path);

  LILV_FOREACH (plugins, p, world->plugins) {
    const LilvPlugin* plugin =
      (const LilvPlugin*)lilv_collection_get((ZixTree*)world->plugins, p);

    // ?new dc:replaces plugin
    if (sord_ask(world->model,
                 NULL,
                 world->uris.dc_replaces,
                 lilv_plugin_get_uri(plugin)->node,
                 NULL)) {
      // TODO: Check if replacement is a known plugin? (expensive)
      ((LilvPlugin*)plugin)->replaced = true;
    }
  }

  // Query out things to cache
  lilv_world_load_specifications(world);
  lilv_world_load_plugin_classes(world);
}

SerdStatus
lilv_world_load_file(LilvWorld* world, SerdReader* reader, const LilvNode* uri)
{
  ZixTreeIter* iter = NULL;
  if (!zix_tree_find((ZixTree*)world->loaded_files, uri, &iter)) {
    return SERD_FAILURE; // File has already been loaded
  }

  size_t               uri_len = 0;
  const uint8_t* const uri_str =
    sord_node_get_string_counted(uri->node, &uri_len);
  if (!!strncmp((const char*)uri_str, "file:", 5)) {
    return SERD_FAILURE; // Not a local file
  }

  if (!!strcmp((const char*)uri_str + uri_len - 4, ".ttl")) {
    return SERD_FAILURE; // Not a Turtle file
  }

  serd_reader_add_blank_prefix(reader, lilv_world_blank_node_prefix(world));
  const SerdStatus st = serd_reader_read_file(reader, uri_str);
  if (st) {
    LILV_ERRORF("Error loading file `%s'\n", lilv_node_as_string(uri));
    return st;
  }

  zix_tree_insert(
    (ZixTree*)world->loaded_files, lilv_node_duplicate(uri), NULL);
  return SERD_SUCCESS;
}

int
lilv_world_load_resource(LilvWorld* world, const LilvNode* resource)
{
  if (!lilv_node_is_uri(resource) && !lilv_node_is_blank(resource)) {
    LILV_ERRORF("Node `%s' is not a resource\n",
                sord_node_get_string(resource->node));
    return -1;
  }

  SordModel* files = lilv_world_filter_model(
    world, world->model, resource->node, world->uris.rdfs_seeAlso, NULL, NULL);

  SordIter* f      = sord_begin(files);
  int       n_read = 0;
  FOREACH_MATCH (f) {
    const SordNode* file      = sord_iter_get_node(f, SORD_OBJECT);
    const uint8_t*  file_str  = sord_node_get_string(file);
    LilvNode*       file_node = lilv_node_new_from_node(world, file);
    if (sord_node_get_type(file) != SORD_URI) {
      LILV_ERRORF("rdfs:seeAlso node `%s' is not a URI\n", file_str);
    } else if (!lilv_world_load_graph(world, (SordNode*)file, file_node)) {
      ++n_read;
    }
    lilv_node_free(file_node);
  }
  sord_iter_free(f);

  sord_free(files);
  return n_read;
}

int
lilv_world_unload_resource(LilvWorld* world, const LilvNode* resource)
{
  if (!lilv_node_is_uri(resource) && !lilv_node_is_blank(resource)) {
    LILV_ERRORF("Node `%s' is not a resource\n",
                sord_node_get_string(resource->node));
    return -1;
  }

  SordModel* files = lilv_world_filter_model(
    world, world->model, resource->node, world->uris.rdfs_seeAlso, NULL, NULL);

  SordIter* f         = sord_begin(files);
  int       n_dropped = 0;
  FOREACH_MATCH (f) {
    const SordNode* file      = sord_iter_get_node(f, SORD_OBJECT);
    LilvNode*       file_node = lilv_node_new_from_node(world, file);
    if (sord_node_get_type(file) != SORD_URI) {
      LILV_ERRORF("rdfs:seeAlso node `%s' is not a URI\n",
                  sord_node_get_string(file));
    } else if (!lilv_world_drop_graph(world, file_node->node)) {
      lilv_world_unload_file(world, file_node);
      ++n_dropped;
    }
    lilv_node_free(file_node);
  }
  sord_iter_free(f);

  sord_free(files);
  return n_dropped;
}

const LilvPluginClass*
lilv_world_get_plugin_class(const LilvWorld* world)
{
  return world->lv2_plugin_class;
}

const LilvPluginClasses*
lilv_world_get_plugin_classes(const LilvWorld* world)
{
  return world->plugin_classes;
}

const LilvPlugins*
lilv_world_get_all_plugins(const LilvWorld* world)
{
  return world->plugins;
}

LilvNode*
lilv_world_get_symbol(LilvWorld* world, const LilvNode* subject)
{
  // Check for explicitly given symbol
  SordNode* snode =
    sord_get(world->model, subject->node, world->uris.lv2_symbol, NULL, NULL);

  if (snode) {
    LilvNode* ret = lilv_node_new_from_node(world, snode);
    sord_node_free(world->world, snode);
    return ret;
  }

  if (!lilv_node_is_uri(subject)) {
    return NULL;
  }

  // Find rightmost segment of URI
  SerdURI uri;
  serd_uri_parse((const uint8_t*)lilv_node_as_uri(subject), &uri);
  const char* str = "_";
  if (uri.fragment.buf) {
    str = (const char*)uri.fragment.buf + 1;
  } else if (uri.query.buf) {
    str = (const char*)uri.query.buf;
  } else if (uri.path.buf) {
    const char* last_slash = strrchr((const char*)uri.path.buf, '/');
    str = last_slash ? (last_slash + 1) : (const char*)uri.path.buf;
  }

  // Replace invalid characters
  const size_t len = strlen(str);
  char* const  sym = (char*)calloc(1, len + 1);
  for (size_t i = 0; i < len; ++i) {
    const char c = str[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_') ||
          (i > 0 && c >= '0' && c <= '9'))) {
      sym[i] = '_';
    } else {
      sym[i] = str[i];
    }
  }

  LilvNode* ret = lilv_new_string(world, sym);
  free(sym);
  return ret;
}
