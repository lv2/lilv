// Copyright 2007-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "lilv_config.h"
#include "lilv_internal.h"
#include "log.h"
#include "node_hash.h"
#include "query.h"
#include "string_util.h"
#include "syntax_skimmer.h"
#include "sys_util.h"
#include "uris.h"

#ifdef LILV_DYN_MANIFEST
#  include "dylib.h"
#  include <lv2/dynmanifest/dynmanifest.h>
#endif

#include <lilv/lilv.h>
#include <serd/serd.h>
#include <sord/sord.h>
#include <zix/allocator.h>
#include <zix/attributes.h>
#include <zix/environment.h>
#include <zix/filesystem.h>
#include <zix/path.h>
#include <zix/status.h>
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

  world->lang           = lilv_get_lang();
  world->specs          = NULL;
  world->plugin_classes = lilv_plugin_classes_new();
  world->plugins        = lilv_plugins_new();
  world->zombies        = lilv_plugins_new();
  world->loaded_files   = lilv_node_hash_new(NULL);

  world->libs = zix_tree_new(NULL, false, lilv_lib_compare, NULL, NULL, NULL);

  lilv_uris_init(&world->uris, world->world);

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

  lilv_uris_cleanup(&world->uris, world->world);

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

  lilv_node_hash_free(world->loaded_files, world->world);
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
  free(world->lang);
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
  } else if (!strcmp(uri, LILV_OPTION_LANG)) {
    if (lilv_node_is_string(value)) {
      free(world->lang);
      world->lang = lilv_normalize_lang(lilv_node_as_string(value));
      return;
    }
  } else if (!strcmp(uri, LILV_OPTION_FILTER_LANG)) {
    if (lilv_node_is_bool(value)) {
      world->opt.filter_language = lilv_node_as_bool(value);
      return;
    }
  } else if (!strcmp(uri, LILV_OPTION_LV2_PATH)) {
    if (lilv_node_is_string(value)) {
      free(world->opt.lv2_path);
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

  return lilv_nodes_from_matches(world,
                                 subject ? subject->node : NULL,
                                 predicate->node,
                                 object ? object->node : NULL,
                                 NULL);
}

const SordNode*
lilv_world_get_unique(LilvWorld* const world,
                      const SordNode*  subject,
                      const SordNode*  predicate)
{
  SordIter* stream = sord_search(world->model, subject, predicate, NULL, NULL);
  if (!stream) {
    return NULL;
  }

  const SordNode* object = sord_iter_get_node(stream, SORD_OBJECT);
  sord_iter_next(stream);
  if (!sord_iter_end(stream)) {
    object = NULL; // Multiple matches => no match
  }

  sord_iter_free(stream);
  return object;
}

LilvNode*
lilv_world_get(LilvWorld*      world,
               const LilvNode* subject,
               const LilvNode* predicate,
               const LilvNode* object)
{
  if (!object) {
    return lilv_node_from_object(world,
                                 subject ? subject->node : NULL,
                                 predicate ? predicate->node : NULL);
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

/// Add all rdfs:seeAlso files of subject to tree
static void
lilv_world_collect_data_files(LilvWorld*            world,
                              const SordNode* const subject,
                              ZixTree* const        tree)
{
  SordIter* files =
    sord_search(world->model, subject, world->uris.rdfs_seeAlso, NULL, NULL);
  FOREACH_MATCH (files) {
    const SordNode* file_node = sord_iter_get_node(files, SORD_OBJECT);
    zix_tree_insert(tree, lilv_node_new_from_node(world, file_node), NULL);
  }
  sord_iter_free(files);
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
  lilv_world_collect_data_files(
    world, specification_node, (ZixTree*)spec->data_uris);

  // Add specification to world specification list
  spec->next   = world->specs;
  world->specs = spec;
}

static void
lilv_world_add_plugin(LilvWorld*      world,
                      const SordNode* plugin_node,
                      const SordNode* manifest_node,
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
    zix_tree_insert((ZixTree*)plugin->data_uris,
                    lilv_node_new_from_node(world, manifest_node),
                    NULL);

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
  lilv_world_collect_data_files(
    world, plugin_node, (ZixTree*)plugin->data_uris);
}

static SerdStatus
lilv_world_load_graph(LilvWorld*      world,
                      const SordNode* graph,
                      const SordNode* uri)
{
  const SerdNode* base = sord_node_to_serd_node(uri);
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
                             const SordNode* manifest_node)
{
#ifdef LILV_DYN_MANIFEST
  if (!world->opt.dyn_manifest) {
    return;
  }

  LV2_Dyn_Manifest_Handle handle = NULL;

  // ?dman a dynman:DynManifest bundle_node
  NodeHash* const manifests =
    lilv_hash_from_matches(world->model,
                           NULL,
                           world->uris.rdf_type,
                           world->uris.dman_DynManifest,
                           bundle_node);
  NODE_HASH_FOREACH (m, manifests) {
    const SordNode* dmanifest = lilv_node_hash_get(manifests, m);

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
      LILV_ERRORF("Failed to open dynmanifest library `%s' (%s)\n",
                  lib_path,
                  dylib_error());
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
    assert(fd);
    get_subjects_func(handle, fd);
    fseek(fd, 0, SEEK_SET);

    // Parse generated data file into the world model
    const SerdNode* base = sord_node_to_serd_node(dmanifest);
    SerdEnv*        env  = serd_env_new(base);
    SerdReader*     reader =
      sord_new_reader(world->model, env, SERD_TURTLE, dmanifest);
    serd_reader_add_blank_prefix(reader, lilv_world_blank_node_prefix(world));
    serd_reader_read_file_handle(reader, fd, (const uint8_t*)"(dyn-manifest)");
    serd_reader_free(reader);
    serd_env_free(env);

    // Close (and automatically delete) temporary data file
    fclose(fd);

    // ?plugin a lv2:Plugin
    NodeHash* const plugins = lilv_hash_from_matches(world->model,
                                                     NULL,
                                                     world->uris.rdf_type,
                                                     world->uris.lv2_Plugin,
                                                     dmanifest);
    NODE_HASH_FOREACH (p, plugins) {
      const SordNode* plug = lilv_node_hash_get(plugins, p);
      lilv_world_add_plugin(world, plug, manifest_node, desc, bundle_node);
    }
    lilv_node_hash_free(plugins, world->world);

    if (desc->refs == 0) {
      lilv_dynmanifest_free(desc);
    }
    lilv_free(lib_path);
  }
  lilv_node_hash_free(manifests, world->world);

#else // LILV_DYN_MANIFEST
  (void)world;
  (void)bundle_node;
  (void)manifest_node;
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

typedef struct {
  SordWorld*      world;
  const SerdNode* lv2_minorVersion;
  const SerdNode* lv2_microVersion;
  const SerdNode* rdfs_seeAlso;
  NodeHash*       see_also;
  LilvVersion     version;
} SkimmedVersion;

static int
int_from_node(const SerdNode* const node)
{
  if (node->type == SERD_LITERAL) {
    const long value = strtol((const char*)node->buf, NULL, 10);
    if (value >= 0 && value < INT_MAX) {
      return (int)value;
    }
  }
  return 0;
}

static SerdStatus
skim_version(SkimmedVersion* const       skimmed,
             SerdEnv* const              env,
             const SerdNode* ZIX_NONNULL subject,
             const SerdNode* ZIX_NONNULL predicate,
             const SerdNode* ZIX_NONNULL object,
             const SerdNode* ZIX_NONNULL object_datatype,
             const SerdNode* ZIX_NONNULL object_lang)
{
  (void)subject;
  (void)object_datatype;
  (void)object_lang;

  SerdNode pred = serd_env_expand_node(env, predicate);

  if (serd_node_equals(&pred, skimmed->rdfs_seeAlso)) {
    if (!skimmed->see_also) {
      skimmed->see_also = lilv_node_hash_new(NULL);
    }
    if (skimmed->see_also) {
      lilv_node_hash_insert(
        skimmed->see_also,
        sord_node_from_serd_node(skimmed->world, env, object, NULL, NULL));
    }
  } else if (serd_node_equals(&pred, skimmed->lv2_minorVersion)) {
    skimmed->version.minor = int_from_node(object);
  } else if (serd_node_equals(&pred, skimmed->lv2_microVersion)) {
    skimmed->version.micro = int_from_node(object);
  }

  serd_node_free(&pred);
  return SERD_SUCCESS;
}

static LilvVersion
load_version(LilvWorld* const      world,
             const SordNode* const bundle_node,
             const SordNode* const resource)
{
  SerdEnv* const env     = serd_env_new(sord_node_to_serd_node(bundle_node));
  SkimmedVersion skimmed = {
    world->world,
    sord_node_to_serd_node(world->uris.lv2_minorVersion),
    sord_node_to_serd_node(world->uris.lv2_microVersion),
    sord_node_to_serd_node(world->uris.rdfs_seeAlso),
    NULL,
    {0, 0}};

  SyntaxSkimmer* const skimmer =
    syntax_skimmer_new(NULL,
                       env,
                       SORD_SUBJECT,
                       sord_node_to_serd_node(resource),
                       &skimmed,
                       (SyntaxSkimmerFunc)skim_version);

  // Read file, recording version numbers and seeAlso files as we go
  SerdReader* const reader       = skimmer->reader;
  uint8_t* const    manifest_uri = lilv_manifest_uri(bundle_node);
  serd_reader_read_file(reader, manifest_uri);
  zix_free(NULL, manifest_uri);

  if (!skimmed.version.minor && !skimmed.version.micro && skimmed.see_also) {
    NODE_HASH_FOREACH (i, skimmed.see_also) {
      const SordNode* file = lilv_node_hash_get(skimmed.see_also, i);
      serd_reader_add_blank_prefix(reader, lilv_world_blank_node_prefix(world));
      serd_reader_read_file(reader, sord_node_get_string(file));
    }
  }

  syntax_skimmer_free(skimmer);
  lilv_node_hash_free(skimmed.see_also, world->world);
  serd_env_free(env);
  return skimmed.version;
}

static int
lilv_world_compare_versions(LilvWorld* const      world,
                            const SordNode* const old_bundle,
                            const SordNode* const new_bundle,
                            const SordNode* const resource)
{
  LilvVersion old_version = load_version(world, old_bundle, resource);
  LilvVersion new_version = load_version(world, new_bundle, resource);
  const int   cmp         = lilv_version_cmp(&new_version, &old_version);
  if (cmp > 0) {
    LILV_WARNF("Loading new version %d.%d of <%s> from <%s>\n",
               new_version.minor,
               new_version.micro,
               sord_node_get_string(resource),
               sord_node_get_string(new_bundle));
    LILV_NOTEF("Replaces version %d.%d from <%s>\n",
               old_version.minor,
               old_version.micro,
               sord_node_get_string(old_bundle));
  } else if (cmp < 0) {
    LILV_WARNF("Ignoring version %d.%d of <%s> from <%s>\n",
               old_version.minor,
               old_version.micro,
               sord_node_get_string(resource),
               sord_node_get_string(old_bundle));
    LILV_NOTEF("Newer version %d.%d loaded from <%s>\n",
               new_version.minor,
               new_version.micro,
               sord_node_get_string(new_bundle));
  }
  return cmp;
}

void
lilv_world_load_bundle(LilvWorld* world, const LilvNode* bundle_uri)
{
  if (!lilv_node_is_uri(bundle_uri)) {
    LILV_ERRORF("Bundle URI `%s' is not a URI\n",
                sord_node_get_string(bundle_uri->node));
    return;
  }

  SordNode* bundle_node  = bundle_uri->node;
  uint8_t*  manifest_uri = lilv_manifest_uri(bundle_node);
  if (!manifest_uri) {
    return;
  }

  LilvNode* const manifest = lilv_new_uri(world, (const char*)manifest_uri);
  zix_free(NULL, manifest_uri);

  // Read manifest into model with graph = bundle_node
  SerdStatus st = lilv_world_load_graph(world, bundle_node, manifest->node);
  if (st > SERD_FAILURE) {
    LILV_ERRORF("Error reading %s\n", lilv_node_as_string(manifest));
    lilv_node_free(manifest);
    return;
  }

  // ?plugin a lv2:Plugin
  SordIter* plug_results = sord_search(world->model,
                                       NULL,
                                       world->uris.rdf_type,
                                       world->uris.lv2_Plugin,
                                       bundle_node);

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
    const int cmp = lilv_world_compare_versions(
      world, last_bundle->node, bundle_uri->node, plugin_uri->node);
    if (cmp > 0) {
      zix_tree_insert(
        (ZixTree*)unload_uris, lilv_node_duplicate(plugin_uri), NULL);
    } else if (cmp < 0) {
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
  plug_results = sord_search(world->model,
                             NULL,
                             world->uris.rdf_type,
                             world->uris.lv2_Plugin,
                             bundle_node);

  FOREACH_MATCH (plug_results) {
    const SordNode* plug = sord_iter_get_node(plug_results, SORD_SUBJECT);
    lilv_world_add_plugin(world, plug, manifest->node, NULL, bundle_node);
  }
  sord_iter_free(plug_results);

  lilv_world_load_dyn_manifest(world, bundle_node, manifest->node);

  // ?spec a lv2:Specification
  // ?spec a owl:Ontology
  const SordNode* spec_preds[] = {
    world->uris.lv2_Specification, world->uris.owl_Ontology, NULL};
  for (const SordNode** p = spec_preds; *p; ++p) {
    SordIter* i =
      sord_search(world->model, NULL, world->uris.rdf_type, *p, bundle_node);
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

int
lilv_world_unload_bundle(LilvWorld* world, const LilvNode* bundle_uri)
{
  if (!bundle_uri) {
    return 0;
  }

  // Remove loaded files entries (the actual file data is dropped below)
  NODE_HASH_FOREACH (i, world->loaded_files) {
    const SordNode* const file = lilv_node_hash_get(world->loaded_files, i);
    if (file && sord_node_equals(file, bundle_uri->node)) {
      lilv_node_hash_remove(world->loaded_files, world->world, file);
    }
  }

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
  LilvWorld* const world = (LilvWorld*)data;
  char* const      path  = zix_path_join(NULL, dir, name);
  if (zix_file_type(path) != ZIX_FILE_TYPE_DIRECTORY) {
    LILV_WARNF("Skipping non-directory `%s' within path entry\n", path);
    free(path);
    return;
  }

  char* const base = zix_path_join(NULL, path, NULL);
  SerdNode    suri = serd_node_new_file_uri((const uint8_t*)base, 0, 0, true);
  LilvNode*   node = lilv_new_uri(world, (const char*)suri.buf);

  lilv_world_load_bundle(world, node);
  lilv_node_free(node);
  serd_node_free(&suri);
  free(base);
  free(path);
}

/** Load all bundles in the directory at `dir_path`. */
static void
lilv_world_load_directory(LilvWorld* world, const char* dir_path)
{
  char* const path = zix_expand_environment_strings(NULL, dir_path);
  if (path) {
    if (zix_file_type(path) == ZIX_FILE_TYPE_DIRECTORY) {
      zix_dir_for_each(path, world, load_dir_entry);
    } else {
      LILV_WARNF("Skipping non-directory `%s' in path\n", path);
    }
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
      lilv_world_load_graph(world, NULL, file->node);
    }
  }
}

static ZixStatus
lilv_world_add_plugin_class(LilvWorld* const      world,
                            const SordNode* const node,
                            const SordNode* const parent)
{
  ZixStatus       st = ZIX_STATUS_NOT_FOUND;
  SordNode* const label =
    sord_get(world->model, node, world->uris.rdfs_label, NULL, NULL);

  if (label) {
    LilvPluginClass* const klass = lilv_plugin_class_new(
      world, parent, node, (const char*)sord_node_get_string(label));
    if (klass) {
      st = zix_tree_insert((ZixTree*)world->plugin_classes, klass, NULL);
      if (st) {
        lilv_plugin_class_free(klass);
      }
    }
  }

  sord_node_free(world->world, label);
  return st;
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
    world->model, NULL, world->uris.rdf_type, world->uris.rdfs_Class, NULL);
  FOREACH_MATCH (classes) {
    const SordNode* class_node = sord_iter_get_node(classes, SORD_SUBJECT);

    SordNode* parent = sord_get(
      world->model, class_node, world->uris.rdfs_subClassOf, NULL, NULL);

    if (parent && sord_node_get_type(parent) == SORD_URI) {
      lilv_world_add_plugin_class(world, class_node, parent);
    }

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

  // Query out things to cache
  lilv_world_load_specifications(world);
  lilv_world_load_plugin_classes(world);
}

SerdStatus
lilv_world_load_file(LilvWorld* world, SerdReader* reader, const SordNode* uri)
{
  assert(uri);
  if (lilv_node_hash_find(world->loaded_files, uri) !=
      lilv_node_hash_end(world->loaded_files)) {
    return SERD_FAILURE; // File has already been loaded
  }

  size_t               uri_len = 0;
  const uint8_t* const uri_str = sord_node_get_string_counted(uri, &uri_len);
  if (!!strncmp((const char*)uri_str, "file:", 5)) {
    return SERD_FAILURE; // Not a local file
  }

  if (!!strcmp((const char*)uri_str + uri_len - 4, ".ttl")) {
    return SERD_FAILURE; // Not a Turtle file
  }

  serd_reader_add_blank_prefix(reader, lilv_world_blank_node_prefix(world));
  const SerdStatus st = serd_reader_read_file(reader, uri_str);
  if (st) {
    LILV_ERRORF("Error loading file `%s'\n", sord_node_get_string(uri));
    return st;
  }

  lilv_node_hash_insert_copy(world->loaded_files, uri);
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
  return lilv_world_load_resource_internal(world, resource->node);
}

int
lilv_world_load_resource_internal(LilvWorld* world, const SordNode* resource)
{
  NodeHash* const files = lilv_hash_from_matches(
    world->model, resource, world->uris.rdfs_seeAlso, NULL, NULL);

  int n_read = 0;
  NODE_HASH_FOREACH (f, files) {
    const SordNode* file = lilv_node_hash_get(files, f);
    if (sord_node_get_type(file) != SORD_URI) {
      LILV_ERRORF("rdfs:seeAlso node `%s' is not a URI\n",
                  sord_node_get_string(file));
    } else if (!lilv_world_load_graph(world, file, file)) {
      ++n_read;
    }
  }

  lilv_node_hash_free(files, world->world);
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

  NodeHash* const files = lilv_hash_from_matches(
    world->model, resource->node, world->uris.rdfs_seeAlso, NULL, NULL);

  int n_dropped = 0;
  NODE_HASH_FOREACH (f, files) {
    const SordNode* file      = lilv_node_hash_get(files, f);
    LilvNode*       file_node = lilv_node_new_from_node(world, file);
    if (sord_node_get_type(file) != SORD_URI) {
      LILV_ERRORF("rdfs:seeAlso node `%s' is not a URI\n",
                  sord_node_get_string(file));
    } else if (!lilv_world_drop_graph(world, file_node->node)) {
      lilv_node_hash_remove(world->loaded_files, world->world, file_node->node);
      ++n_dropped;
    }
    lilv_node_free(file_node);
  }

  lilv_node_hash_free(files, world->world);
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
