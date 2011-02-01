/* SLV2
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <dirent.h>
#include <wordexp.h>
#include <string.h>
#ifdef SLV2_DYN_MANIFEST
#include <dlfcn.h>
#endif
#include <redland.h>
#include "slv2/types.h"
#include "slv2/world.h"
#include "slv2/slv2.h"
#include "slv2/util.h"
#include "slv2-config.h"
#include "slv2_internal.h"

/* private */
static SLV2World
slv2_world_new_internal(SLV2World world)
{
	assert(world);
	assert(world->world);

	world->storage = slv2_world_new_storage(world);
	if (!world->storage)
		goto fail;

	world->model = librdf_new_model(world->world, world->storage, NULL);
	if (!world->model)
		goto fail;

	world->parser = librdf_new_parser(world->world, "turtle", NULL, NULL);
	if (!world->parser)
		goto fail;

	world->plugin_classes = slv2_plugin_classes_new();

	world->plugins = slv2_plugins_new();

#define NS_DYNMAN (const uint8_t*)"http://lv2plug.in/ns/ext/dynmanifest#"

#define NEW_URI(uri) librdf_new_node_from_uri_string(world->world, uri);

	world->dyn_manifest_node       = NEW_URI(NS_DYNMAN    "DynManifest");
	world->lv2_specification_node  = NEW_URI(SLV2_NS_LV2  "Specification");
	world->lv2_plugin_node         = NEW_URI(SLV2_NS_LV2  "Plugin");
	world->lv2_binary_node         = NEW_URI(SLV2_NS_LV2  "binary");
	world->lv2_default_node        = NEW_URI(SLV2_NS_LV2  "default");
	world->lv2_minimum_node        = NEW_URI(SLV2_NS_LV2  "minimum");
	world->lv2_maximum_node        = NEW_URI(SLV2_NS_LV2  "maximum");
	world->lv2_port_node           = NEW_URI(SLV2_NS_LV2  "port");
	world->lv2_portproperty_node   = NEW_URI(SLV2_NS_LV2  "portProperty");
	world->lv2_reportslatency_node = NEW_URI(SLV2_NS_LV2  "reportsLatency");
	world->lv2_index_node          = NEW_URI(SLV2_NS_LV2  "index");
	world->lv2_symbol_node         = NEW_URI(SLV2_NS_LV2  "symbol");
	world->rdf_a_node              = NEW_URI(SLV2_NS_RDF  "type");
	world->rdf_value_node          = NEW_URI(SLV2_NS_RDF  "value");
	world->rdfs_class_node         = NEW_URI(SLV2_NS_RDFS "Class");
	world->rdfs_label_node         = NEW_URI(SLV2_NS_RDFS "label");
	world->rdfs_seealso_node       = NEW_URI(SLV2_NS_RDFS "seeAlso");
	world->rdfs_subclassof_node    = NEW_URI(SLV2_NS_RDFS "subClassOf");
	world->slv2_bundleuri_node     = NEW_URI(SLV2_NS_SLV2 "bundleURI");
	world->slv2_dmanifest_node     = NEW_URI(SLV2_NS_SLV2 "dynamic-manifest");
	world->xsd_integer_node        = NEW_URI(SLV2_NS_XSD  "integer");
	world->xsd_decimal_node        = NEW_URI(SLV2_NS_XSD  "decimal");

	world->lv2_plugin_class = slv2_plugin_class_new(
		world, NULL, world->lv2_plugin_node, "Plugin");

	world->namespaces = librdf_new_hash_from_string(
		world->world, NULL,
		"rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#',"
		"rdfs='http://www.w3.org/2000/01/rdf-schema#',"
		"doap='http://usefulinc.com/ns/doap#',"
		"foaf='http://xmlns.com/foaf/0.1/',"
		"lv2='http://lv2plug.in/ns/lv2core#',"
		"lv2ev='http://lv2plug.in/ns/ext/event#'");

	return world;

fail:
	/* keep on rockin' in the */ free(world);
	return NULL;
}


/* private */
librdf_storage*
slv2_world_new_storage(SLV2World world)
{
	static bool warned = false;
	librdf_hash* options = librdf_new_hash_from_string(world->world, NULL,
			"index-spo='yes',index-ops='yes'");
	librdf_storage* ret = librdf_new_storage_with_options(
			world->world, "trees", NULL, options);
	if (!ret) {
		warned = true;
		SLV2_WARN("Unable to create \"trees\" RDF storage, you should upgrade librdf.\n");
		ret = librdf_new_storage(world->world, "hashes", NULL,
				"hash-type='memory'");
	}

	librdf_free_hash(options);
	return ret;
}


SLV2World
slv2_world_new()
{
	SLV2World world = (SLV2World)malloc(sizeof(struct _SLV2World));

	world->world = librdf_new_world();
	if (!world->world) {
		free(world);
		return NULL;
	}

	world->local_world = true;

	librdf_world_open(world->world);

	return slv2_world_new_internal(world);
}


SLV2World
slv2_world_new_using_rdf_world(librdf_world* rdf_world)
{
	if (rdf_world == NULL)
		return slv2_world_new();

	SLV2World world = (SLV2World)malloc(sizeof(struct _SLV2World));

	world->world = rdf_world;
	world->local_world = false;

	return slv2_world_new_internal(world);
}


void
slv2_world_free(SLV2World world)
{
	slv2_plugin_class_free(world->lv2_plugin_class);
	world->lv2_plugin_class = NULL;

	slv2_node_free(world->dyn_manifest_node);
	slv2_node_free(world->lv2_specification_node);
	slv2_node_free(world->lv2_plugin_node);
	slv2_node_free(world->lv2_binary_node);
	slv2_node_free(world->lv2_default_node);
	slv2_node_free(world->lv2_minimum_node);
	slv2_node_free(world->lv2_maximum_node);
	slv2_node_free(world->lv2_port_node);
	slv2_node_free(world->lv2_portproperty_node);
	slv2_node_free(world->lv2_reportslatency_node);
	slv2_node_free(world->lv2_index_node);
	slv2_node_free(world->lv2_symbol_node);
	slv2_node_free(world->rdf_a_node);
	slv2_node_free(world->rdf_value_node);
	slv2_node_free(world->rdfs_label_node);
	slv2_node_free(world->rdfs_seealso_node);
	slv2_node_free(world->rdfs_subclassof_node);
	slv2_node_free(world->rdfs_class_node);
	slv2_node_free(world->slv2_bundleuri_node);
	slv2_node_free(world->slv2_dmanifest_node);
	slv2_node_free(world->xsd_integer_node);
	slv2_node_free(world->xsd_decimal_node);

	for (int i=0; i < raptor_sequence_size(world->plugins); ++i)
		slv2_plugin_free(raptor_sequence_get_at(world->plugins, i));
	raptor_free_sequence(world->plugins);
	world->plugins = NULL;

	raptor_free_sequence(world->plugin_classes);
	world->plugin_classes = NULL;

	librdf_free_parser(world->parser);
	world->parser = NULL;

	librdf_free_model(world->model);
	world->model = NULL;

	librdf_free_storage(world->storage);
	world->storage = NULL;

	librdf_free_hash(world->namespaces);

	if (world->local_world)
		librdf_free_world(world->world);

	world->world = NULL;

	free(world);
}


/** Load the entire contents of a file into the world model.
 */
void
slv2_world_load_file(SLV2World world, librdf_uri* file_uri)
{
	librdf_parser_parse_into_model(world->parser, file_uri, file_uri, world->model);
}

static SLV2Matches
slv2_world_find_statements(SLV2World     world,
                           librdf_model* model,
                           SLV2Node      subject,
                           SLV2Node      predicate,
                           SLV2Node      object)
{
	librdf_statement* q = librdf_new_statement_from_nodes(
		world->world,
		subject ? slv2_node_copy(subject) : NULL,
		predicate ? slv2_node_copy(predicate) : NULL,
		object ? slv2_node_copy(object) : NULL);
	SLV2Matches results = librdf_model_find_statements(model, q);
	librdf_free_statement(q);
	return results;
}

void
slv2_world_load_bundle(SLV2World world, SLV2Value bundle_uri)
{
	if (!slv2_value_is_uri(bundle_uri)) {
		SLV2_ERROR("Bundle 'URI' is not a URI\n");
		return;
	}

	librdf_uri* manifest_uri = librdf_new_uri_relative_to_base(
		librdf_node_get_uri(bundle_uri->val.uri_val),
		(const uint8_t*)"manifest.ttl");

	/* Parse the manifest into a temporary model */
	librdf_storage* manifest_storage = slv2_world_new_storage(world);

	librdf_model* manifest_model = librdf_new_model(world->world,
			manifest_storage, NULL);
	librdf_parser_parse_into_model(world->parser, manifest_uri,
			manifest_uri, manifest_model);

#ifdef SLV2_DYN_MANIFEST
	typedef void* LV2_Dyn_Manifest_Handle;
	LV2_Dyn_Manifest_Handle handle = NULL;

	SLV2Matches dmanifests = slv2_world_find_statements(
		world, world->model,
		NULL,
		world->rdf_a_node,
		world->dyn_manifest_node);
	FOREACH_MATCH(dmanifests) {
		SLV2Node    dmanifest = slv2_match_subject(dmanifests);
		SLV2Matches binaries  = slv2_world_find_statements(
			world, world->model,
			dmanifest,
			world->lv2_binary_node,
			NULL);
		if (slv2_matches_end(binaries)) {
			slv2_match_end(binaries);
			SLV2_ERRORF("Dynamic manifest in <%s> has no binaries, ignored\n",
			            slv2_value_as_uri(bundle_uri));
			continue;
		}

		SLV2Node       binary   = slv2_node_copy(slv2_match_object(binaries));
		const uint8_t* lib_uri  = librdf_uri_as_string(librdf_node_get_uri(binary));
		const char*    lib_path = slv2_uri_to_path((const char*)lib_uri);
		if (!lib_path)
			continue;

		void* lib = dlopen(lib_path, RTLD_LAZY);
		if (!lib)
			continue;

		// Open dynamic manifest
		typedef int (*OpenFunc)(LV2_Dyn_Manifest_Handle*, const LV2_Feature *const *);
		OpenFunc open_func = (OpenFunc)dlsym(lib, "lv2_dyn_manifest_open");
		if (open_func)
			open_func(&handle, &dman_features);

		// Get subjects (the data that would be in manifest.ttl)
		typedef int (*GetSubjectsFunc)(LV2_Dyn_Manifest_Handle, FILE*);
		GetSubjectsFunc get_subjects_func = (GetSubjectsFunc)dlsym(lib,
				"lv2_dyn_manifest_get_subjects");
		if (!get_subjects_func)
			continue;

		librdf_storage* dyn_manifest_storage = slv2_world_new_storage(world);
		librdf_model*   dyn_manifest_model   = librdf_new_model(world->world,
			dyn_manifest_storage, NULL);

		FILE* fd = tmpfile();
		get_subjects_func(handle, fd);
		rewind(fd);
		librdf_parser_parse_file_handle_into_model(
			world->parser, fd, 0,
			librdf_node_get_uri(bundle_uri->val.uri_val),
			dyn_manifest_model);
		fclose(fd);

		// ?plugin a lv2:Plugin
		SLV2Matches dyn_plugins = slv2_world_find_statements(
			world, dyn_manifest_model,
			NULL,
			world->rdf_a_node,
			world->lv2_plugin_node);
		FOREACH_MATCH(dyn_plugins) {
			SLV2Node plugin = slv2_match_subject(dyn_plugins);

			// Add ?plugin slv2:dynamic-manifest ?binary to dynamic model
			librdf_model_add(
				manifest_model, plugin,
				librdf_new_node_from_uri_string(world->world, SLV2_NS_RDFS "seeAlso"),
				slv2_node_copy(binary));
		}
		slv2_match_end(dyn_plugins);

		// Merge dynamic model into main manifest model
		librdf_stream* dyn_manifest_stream = librdf_model_as_stream(dyn_manifest_model);
		librdf_model_add_statements(manifest_model, dyn_manifest_stream);
		librdf_free_stream(dyn_manifest_stream);
		librdf_free_model(dyn_manifest_model);
		librdf_free_storage(dyn_manifest_storage);
	}
	slv2_match_end(dmanifests);
#endif // SLV2_DYN_MANIFEST

	// ?plugin a lv2:Plugin
	SLV2Matches results = slv2_world_find_statements(
		world, manifest_model,
		NULL,
		world->rdf_a_node,
		world->lv2_plugin_node);
	FOREACH_MATCH(results) {
		SLV2Node plugin = slv2_match_subject(results);

		// Add ?plugin rdfs:seeAlso <manifest.ttl>
		librdf_model_add(
			world->model,
			slv2_node_copy(plugin),
			librdf_new_node_from_uri_string(world->world, SLV2_NS_RDFS "seeAlso"),
			librdf_new_node_from_uri(world->world, manifest_uri));

		// Add ?plugin slv2:bundleURI <file://some/path>
		librdf_model_add(
			world->model,
			slv2_node_copy(plugin),
			librdf_new_node_from_uri_string(world->world, SLV2_NS_SLV2 "bundleURI"),
			slv2_node_copy(bundle_uri->val.uri_val));
	}
	slv2_match_end(results);

	// ?specification a lv2:Specification
	results = slv2_world_find_statements(
		world, manifest_model,
		NULL,
		world->rdf_a_node,
		world->lv2_specification_node);
	FOREACH_MATCH(results) {
		SLV2Node spec = slv2_match_subject(results);

		// Add ?specification rdfs:seeAlso <manifest.ttl>
		librdf_model_add(
			world->model,
			slv2_node_copy(spec),
			librdf_new_node_from_uri_string(world->world, SLV2_NS_RDFS "seeAlso"),
			librdf_new_node_from_uri(world->world, manifest_uri));

		// Add ?specification slv2:bundleURI <file://some/path>
		librdf_model_add(
			world->model,
			slv2_node_copy(spec),
			librdf_new_node_from_uri_string(world->world, SLV2_NS_SLV2 "bundleURI"),
			slv2_node_copy(bundle_uri->val.uri_val));
	}
	slv2_match_end(results);

	// Join the temporary model to the main model
	librdf_stream* manifest_stream = librdf_model_as_stream(manifest_model);
	librdf_model_add_statements(world->model, manifest_stream);
	librdf_free_stream(manifest_stream);

	librdf_free_model(manifest_model);
	librdf_free_storage(manifest_storage);
	librdf_free_uri(manifest_uri);
}


/** Load all bundles under a directory.
 * Private.
 */
void
slv2_world_load_directory(SLV2World world, const char* dir)
{
	DIR* pdir = opendir(dir);
	if (!pdir)
		return;

	struct dirent* pfile;
	while ((pfile = readdir(pdir))) {
		if (!strcmp(pfile->d_name, ".") || !strcmp(pfile->d_name, ".."))
			continue;

		char* uri = slv2_strjoin("file://", dir, "/", pfile->d_name, "/", NULL);

		// FIXME: Probably a better way to check if a dir exists
		DIR* bundle_dir = opendir(uri + 7);

		if (bundle_dir != NULL) {
			closedir(bundle_dir);
			SLV2Value uri_val = slv2_value_new_uri(world, uri);
			slv2_world_load_bundle(world, uri_val);
			slv2_value_free(uri_val);
		}

		free(uri);
	}

	closedir(pdir);
}


void
slv2_world_load_path(SLV2World   world,
                     const char* lv2_path)
{
	char* path = slv2_strjoin(lv2_path, ":", NULL);
	char* dir  = path; // Pointer into path

	// Go through string replacing ':' with '\0', using the substring,
	// then replacing it with 'X' and moving on.  i.e. strtok on crack.
	while (strchr(path, ':') != NULL) {
		char* delim = strchr(path, ':');
		*delim = '\0';

		slv2_world_load_directory(world, dir);

		*delim = 'X';
		dir = delim + 1;
	}

	free(path);
}


/** Comparator for sorting SLV2Plugins */
int
slv2_plugin_compare_by_uri(const void* a, const void* b)
{
    SLV2Plugin plugin_a = *(SLV2Plugin*)a;
    SLV2Plugin plugin_b = *(SLV2Plugin*)b;

    return strcmp(slv2_value_as_uri(plugin_a->plugin_uri),
                  slv2_value_as_uri(plugin_b->plugin_uri));
}


/** Comparator for sorting SLV2PluginClasses */
int
slv2_plugin_class_compare_by_uri(const void* a, const void* b)
{
    SLV2PluginClass class_a = *(SLV2PluginClass*)a;
    SLV2PluginClass class_b = *(SLV2PluginClass*)b;

    return strcmp(slv2_value_as_uri(class_a->uri),
                  slv2_value_as_uri(class_b->uri));
}


void
slv2_world_load_specifications(SLV2World world)
{
	SLV2Matches specs = slv2_world_find_statements(
		world, world->model,
		NULL,
		world->rdf_a_node,
		world->lv2_specification_node);
	FOREACH_MATCH(specs) {
		SLV2Node    spec_node = slv2_match_subject(specs);
		SLV2Matches files     = slv2_world_find_statements(
			world, world->model,
			spec_node,
			world->rdfs_seealso_node,
			NULL);
		FOREACH_MATCH(files) {
			SLV2Node file_node = slv2_match_object(files);
			slv2_world_load_file(world, librdf_node_get_uri(file_node));
		}
		slv2_match_end(files);
	}
	slv2_match_end(specs);
}


void
slv2_world_load_plugin_classes(SLV2World world)
{
	/* FIXME: This loads all classes, not just lv2:Plugin subclasses.
	   However, if the host gets all the classes via slv2_plugin_class_get_children
	   starting with lv2:Plugin as the root (which is e.g. how a host would build
	   a menu), they won't be seen anyway...
	*/

	SLV2Matches classes = slv2_world_find_statements(
		world, world->model,
		NULL,
		world->rdf_a_node,
		world->rdfs_class_node);
	FOREACH_MATCH(classes) {
		SLV2Node class_node = slv2_match_subject(classes);

		// Get parents (superclasses)
		SLV2Matches parents = slv2_world_find_statements(
			world, world->model,
			class_node,
			world->rdfs_subclassof_node,
			NULL);

		if (slv2_matches_end(parents)) {
			slv2_match_end(parents);
			continue;
		}

		SLV2Node parent_node = slv2_node_copy(slv2_match_object(parents));
		slv2_match_end(parents);

		if (!librdf_node_is_resource(parent_node)) {
			// Class parent is not a resource, ignore (e.g. owl restriction)
			continue;
		}

		// Get labels
		SLV2Matches labels = slv2_world_find_statements(
			world, world->model,
			class_node,
			world->rdfs_label_node,
			NULL);

		if (slv2_matches_end(labels)) {
			slv2_match_end(labels);
			continue;
		}

		SLV2Node       label_node = slv2_node_copy(slv2_match_object(labels));
		const uint8_t* label      = librdf_node_get_literal_value(label_node);
		slv2_match_end(labels);

		SLV2PluginClasses classes   = world->plugin_classes;
		const unsigned    n_classes = raptor_sequence_size(classes);
#ifndef NDEBUG
		if (n_classes > 0) {
			// Class results are in increasing sorted order
			SLV2PluginClass prev = raptor_sequence_get_at(classes, n_classes - 1);
			assert(strcmp(
				       slv2_value_as_string(slv2_plugin_class_get_uri(prev)),
				       (const char*)librdf_uri_as_string(
					       librdf_node_get_uri(class_node))) < 0);
		}
#endif
		raptor_sequence_push(classes,
		                     slv2_plugin_class_new(world,
		                                           parent_node,
		                                           class_node,
		                                           (const char*)label));

		slv2_node_free(parent_node);
		slv2_node_free(label_node);
	}
	slv2_match_end(classes);
}


void
slv2_world_load_all(SLV2World world)
{
	char* lv2_path = getenv("LV2_PATH");

	/* 1. Read all manifest files into model */

	if (lv2_path) {
		slv2_world_load_path(world, lv2_path);
	} else {
		char default_path[sizeof(SLV2_DEFAULT_LV2_PATH)];
		memcpy(default_path, SLV2_DEFAULT_LV2_PATH, sizeof(SLV2_DEFAULT_LV2_PATH));
		char* dir = default_path;
		size_t lv2_path_len = 0;
		while (*dir != '\0') {
			char* colon = strchr(dir, ':');
			if (colon)
				*colon = '\0';
			wordexp_t p;
			wordexp(dir, &p, 0);
			for (unsigned i = 0; i < p.we_wordc; i++) {
				const size_t word_len = strlen(p.we_wordv[i]);
				if (!lv2_path) {
					lv2_path = strdup(p.we_wordv[i]);
					lv2_path_len = word_len;
				} else {
					lv2_path = (char*)realloc(lv2_path, lv2_path_len + word_len + 2);
					*(lv2_path + lv2_path_len) = ':';
					strcpy(lv2_path + lv2_path_len + 1, p.we_wordv[i]);
					lv2_path_len += word_len + 1;
				}
			}
			wordfree(&p);
			if (colon)
				dir = colon + 1;
			else
				*dir = '\0';
		}

		slv2_world_load_path(world, lv2_path);

		free(lv2_path);
	}


	/* 2. Query out things to cache */

	slv2_world_load_specifications(world);

	slv2_world_load_plugin_classes(world);

	SLV2Matches plugins = slv2_world_find_statements(
		world, world->model,
		NULL,
		world->rdf_a_node,
		world->lv2_plugin_node);
	FOREACH_MATCH(plugins) {
		SLV2Node    plugin_node = slv2_match_subject(plugins);
		const char* plugin_uri  = (const char*)librdf_uri_as_string(
			librdf_node_get_uri(plugin_node));

		SLV2Matches bundles = slv2_world_find_statements(
			world, world->model,
			plugin_node,
			world->slv2_bundleuri_node,
			NULL);

		if (slv2_matches_end(bundles)) {
			slv2_match_end(bundles);
			SLV2_ERRORF("Plugin <%s> has no bundle, ignored\n", plugin_uri);
			continue;
		}
		
		SLV2Node bundle_node = slv2_node_copy(slv2_match_object(bundles));

		slv2_matches_next(bundles);
		if (!slv2_matches_end(bundles)) {
			slv2_match_end(bundles);
			SLV2_ERRORF("Plugin <%s> found in several bundles, ignored\n",
			            plugin_uri);
			continue;
		}

		slv2_match_end(bundles);

		// Add a new plugin to the world
		SLV2Value      uri       = slv2_value_new_from_node(world, plugin_node);
		const unsigned n_plugins = raptor_sequence_size(world->plugins);
#ifndef NDEBUG
		if (n_plugins > 0) {
			// Plugin results are in increasing sorted order
			SLV2Plugin prev = raptor_sequence_get_at(world->plugins, n_plugins - 1);
			assert(strcmp(slv2_value_as_string(slv2_plugin_get_uri(prev)),
			              plugin_uri) < 0);
		}
#endif

		SLV2Plugin plugin = slv2_plugin_new(
			world, uri, slv2_value_new_from_node(world, bundle_node));

		raptor_sequence_push(world->plugins, plugin);

#ifdef SLV2_DYN_MANIFEST
		{
			SLV2Matches dmanifests = slv2_world_find_statements(
				world, world->model,
				plugin_node,
				world->slv2_dmanifest_node,
				NULL);
			FOREACH_MATCH(dmanifests) {
				SLV2Node    lib_node = slv2_match_object(dmanifests);
				const char* lib_uri  = (const char*)librdf_uri_as_string(
					librdf_node_get_uri(lib_node));

				if (dlopen(slv2_uri_to_path(lib_uri, RTLD_LAZY))) {
					plugin->dynman_uri = slv2_value_new_from_node(world, lib_node);
				}
			}
			slv2_match_end(dmanifests);
		}
#endif
		SLV2Matches files = slv2_world_find_statements(
			world, world->model,
			plugin_node,
			world->rdfs_seealso_node,
			NULL);
		FOREACH_MATCH(files) {
			SLV2Node file_node = slv2_match_object(files);
			raptor_sequence_push(plugin->data_uris,
			                     slv2_value_new_from_node(world, file_node));
		}
		slv2_match_end(files);
		
		slv2_node_free(bundle_node);
	}
	slv2_match_end(plugins);
}


SLV2PluginClass
slv2_world_get_plugin_class(SLV2World world)
{
	return world->lv2_plugin_class;
}


SLV2PluginClasses
slv2_world_get_plugin_classes(SLV2World world)
{
	return world->plugin_classes;
}


SLV2Plugins
slv2_world_get_all_plugins(SLV2World world)
{
	return world->plugins;
}


SLV2Plugins
slv2_world_get_plugins_by_filter(SLV2World world, bool (*include)(SLV2Plugin))
{
	SLV2Plugins result = slv2_plugins_new();

	for (int i=0; i < raptor_sequence_size(world->plugins); ++i) {
		SLV2Plugin p = raptor_sequence_get_at(world->plugins, i);
		if (include(p))
			raptor_sequence_push(result, p);
	}

	return result;
}

