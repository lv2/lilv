/* SLV2
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include CONFIG_H_PATH

#define _XOPEN_SOURCE 500
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <librdf.h>
#include <slv2/world.h>
#include <slv2/slv2.h>
#include <slv2/util.h>
#include "slv2_internal.h"


SLV2World
slv2_world_new()
{
	SLV2World world = (SLV2World)malloc(sizeof(struct _SLV2World));

	world->world = librdf_new_world();
	if (!world->world)
		goto fail;
	
	world->local_world = true;

	librdf_world_open(world->world);
	
	world->storage = librdf_new_storage(world->world, "hashes", NULL,
			"hash-type='memory'");
	if (!world->storage)
		goto fail;

	world->model = librdf_new_model(world->world, world->storage, NULL);
	if (!world->model)
		goto fail;

	world->parser = librdf_new_parser(world->world, "turtle", NULL, NULL);
	if (!world->parser)
		goto fail;

	world->plugin_classes = slv2_plugin_classes_new();
	
	// Add the ever-present lv2:Plugin to classes
	static const char* lv2_plugin_uri = "http://lv2plug.in/ns/lv2core#Plugin";
	raptor_sequence_push(world->plugin_classes, slv2_plugin_class_new(
				world, NULL, lv2_plugin_uri, "Plugin"));
	
	world->plugins = slv2_plugins_new();
	
	world->lv2_specification_node = librdf_new_node_from_uri_string(world->world,
			(unsigned char*)"http://lv2plug.in/ns/lv2core#Specification");
	
	world->lv2_plugin_node = librdf_new_node_from_uri_string(world->world,
			(unsigned char*)"http://lv2plug.in/ns/lv2core#Plugin");
	
	world->rdf_a_node = librdf_new_node_from_uri_string(world->world,
			(unsigned char*)"http://www.w3.org/1999/02/22-rdf-syntax-ns#type");

	world->rdf_lock = NULL;
	world->rdf_unlock = NULL;
	world->rdf_lock_data = NULL;
	world->rdf_lock_count = 0;
	
	return world;

fail:
	free(world);
	return NULL;
}


SLV2World
slv2_world_new_using_rdf_world(librdf_world* rdf_world)
{
	SLV2World world = (SLV2World)malloc(sizeof(struct _SLV2World));

	world->world = rdf_world;
	if (!world->world)
		goto fail;
	
	world->local_world = false;

	world->storage = librdf_new_storage(world->world, "hashes", NULL,
			"hash-type='memory'");
	if (!world->storage)
		goto fail;

	world->model = librdf_new_model(world->world, world->storage, NULL);
	if (!world->model)
		goto fail;

	world->parser = librdf_new_parser(world->world, "turtle", NULL, NULL);
	if (!world->parser)
		goto fail;

	world->plugin_classes = slv2_plugin_classes_new();
	
	// Add the ever-present lv2:Plugin to classes
	static const char* lv2_plugin_uri = "http://lv2plug.in/ns/lv2core#Plugin";
	raptor_sequence_push(world->plugin_classes, slv2_plugin_class_new(
				world, NULL, lv2_plugin_uri, "Plugin"));
	
	world->plugins = slv2_plugins_new();
	
	world->lv2_specification_node = librdf_new_node_from_uri_string(world->world,
			(unsigned char*)"http://lv2plug.in/ns/lv2core#Specification");
	
	world->lv2_plugin_node = librdf_new_node_from_uri_string(rdf_world,
			(unsigned char*)"http://lv2plug.in/ns/lv2core#Plugin");
	
	world->rdf_a_node = librdf_new_node_from_uri_string(rdf_world,
			(unsigned char*)"http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	
	world->rdf_lock = NULL;
	world->rdf_unlock = NULL;
	world->rdf_lock_data = NULL;
	world->rdf_lock_count = 0;

	return world;

fail:
	free(world);
	return NULL;
}


void
slv2_world_free(SLV2World world)
{
	if (world->rdf_lock)
		world->rdf_lock(world->rdf_lock_data);

	librdf_free_node(world->lv2_specification_node);
	librdf_free_node(world->lv2_plugin_node);
	librdf_free_node(world->rdf_a_node);

	for (int i=0; i < raptor_sequence_size(world->plugins); ++i)
		slv2_plugin_free(raptor_sequence_get_at(world->plugins, i));
	raptor_free_sequence(world->plugins);
	world->plugins = NULL;
	
	slv2_plugin_classes_free(world->plugin_classes);
	world->plugin_classes = NULL;
	
	librdf_free_parser(world->parser);
	world->parser = NULL;
	
	librdf_free_model(world->model);
	world->model = NULL;
	
	librdf_free_storage(world->storage);
	world->storage = NULL;
	
	if (world->local_world)
		librdf_free_world(world->world);

	world->world = NULL;
	
	if (world->rdf_unlock)
		world->rdf_unlock(world->rdf_lock_data);

	free(world);
}


void
slv2_world_set_rdf_lock_function(SLV2World world, void (*lock)(void*), void* data)
{
	world->rdf_lock = lock;
	world->rdf_lock_data = data;
}


void
slv2_world_set_rdf_unlock_function(SLV2World world, void (*unlock)(void*))
{
	world->rdf_unlock = unlock;
}


void
slv2_world_lock_if_necessary(SLV2World world)
{
	if (world->rdf_lock) {
	
		if (world->rdf_lock_count == 0)
			world->rdf_lock(world->rdf_lock_data);

		++world->rdf_lock_count;
	
	}
}


void
slv2_world_unlock_if_necessary(SLV2World world)
{
	if (world->rdf_lock && world->rdf_lock_count > 0) {

		if (world->rdf_lock_count == 1 && world->rdf_unlock)
			world->rdf_unlock(world->rdf_lock_data);

		world->rdf_lock_count = 0;

	}
}


/** Load the entire contents of a file into the world model.
 */
void
slv2_world_load_file(SLV2World world, librdf_uri* file_uri)
{
	slv2_world_lock_if_necessary(world);
	
	librdf_storage* storage = librdf_new_storage(world->world, 
			"memory", NULL, NULL);
	librdf_model* model = librdf_new_model(world->world,
			storage, NULL);
	librdf_parser_parse_into_model(world->parser, file_uri, NULL, 
			model);

	librdf_stream* stream = librdf_model_as_stream(model);
	librdf_model_add_statements(world->model, stream);
	librdf_free_stream(stream);
	
	librdf_free_model(model);
	librdf_free_storage(storage);

	slv2_world_unlock_if_necessary(world);
}



void
slv2_world_load_bundle(SLV2World world, const char* bundle_uri_str)
{
	slv2_world_lock_if_necessary(world);

	librdf_uri* bundle_uri = librdf_new_uri(world->world,
			(const unsigned char*)bundle_uri_str);

	librdf_uri* manifest_uri = librdf_new_uri_relative_to_base(
			bundle_uri, (const unsigned char*)"manifest.ttl");

	/* Parse the manifest into a temporary model */
	librdf_storage* manifest_storage = librdf_new_storage(world->world, 
			"memory", NULL, NULL);
	librdf_model* manifest_model = librdf_new_model(world->world,
			manifest_storage, NULL);
	librdf_parser_parse_into_model(world->parser, manifest_uri, NULL, 
			manifest_model);

	/* Query statement: ?plugin a lv2:Plugin */
	librdf_statement* q = librdf_new_statement_from_nodes(world->world,
			NULL, world->rdf_a_node, world->lv2_plugin_node);

	librdf_stream* results = librdf_model_find_statements(manifest_model, q);

	while (!librdf_stream_end(results)) {
		librdf_statement* s = librdf_stream_get_object(results);

		librdf_node* plugin_node = librdf_new_node_from_node(librdf_statement_get_subject(s));

		/* Add ?plugin rdfs:seeAlso <manifest.ttl>*/
		librdf_node* subject = plugin_node;
		librdf_node* predicate = librdf_new_node_from_uri_string(world->world, 
				(unsigned char*)"http://www.w3.org/2000/01/rdf-schema#seeAlso");
		librdf_node* object = librdf_new_node_from_uri(world->world,
				manifest_uri);

		librdf_model_add(world->model, subject, predicate, object);
		
		/* Add ?plugin slv2:bundleURI <file://some/path> */
		subject = librdf_new_node_from_node(plugin_node);
		predicate = librdf_new_node_from_uri_string(world->world, 
				(unsigned char*)"http://drobilla.net/ns/slv2#bundleURI");
		object = librdf_new_node_from_uri(world->world, bundle_uri);

		librdf_model_add(world->model, subject, predicate, object);

		librdf_stream_next(results);
	}
	
	librdf_free_stream(results);
	
	/* Query statement: ?specification a lv2:Specification */
	q = librdf_new_statement_from_nodes(world->world,
			NULL, world->rdf_a_node, world->lv2_specification_node);

	results = librdf_model_find_statements(manifest_model, q);

	while (!librdf_stream_end(results)) {
		librdf_statement* s = librdf_stream_get_object(results);

		librdf_node* spec_node = librdf_new_node_from_node(librdf_statement_get_subject(s));

		/* Add ?specification rdfs:seeAlso <manifest.ttl> */
		librdf_node* subject = spec_node;
		librdf_node* predicate = librdf_new_node_from_uri_string(world->world, 
				(unsigned char*)"http://www.w3.org/2000/01/rdf-schema#seeAlso");
		librdf_node* object = librdf_new_node_from_uri(world->world,
				manifest_uri);
		
		librdf_model_add(world->model, subject, predicate, object);
		
		/* Add ?specification slv2:bundleURI <file://some/path> */
		subject = librdf_new_node_from_node(spec_node);
		predicate = librdf_new_node_from_uri_string(world->world, 
				(unsigned char*)"http://drobilla.net/ns/slv2#bundleURI");
		object = librdf_new_node_from_uri(world->world, bundle_uri);

		librdf_model_add(world->model, subject, predicate, object);

		librdf_stream_next(results);
	}
	
	librdf_free_stream(results);
	
	/* Join the temporary model to the main model */
	librdf_stream* manifest_stream = librdf_model_as_stream(manifest_model);
	librdf_model_add_statements(world->model, manifest_stream);
	librdf_free_stream(manifest_stream);

	librdf_free_model(manifest_model);
	free(q);
	librdf_free_storage(manifest_storage);
	librdf_free_uri(manifest_uri);
	librdf_free_uri(bundle_uri);
	
	slv2_world_unlock_if_necessary(world);
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
			slv2_world_load_bundle(world, uri);
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


/** comparator for sorting */
/*int
slv2_plugin_compare_by_uri(const void* a, const void* b)
{
	SLV2Plugin plugin_a = *(SLV2Plugin*)a;
	SLV2Plugin plugin_b = *(SLV2Plugin*)b;

	return strcmp((const char*)librdf_uri_as_string(plugin_a->plugin_uri),
	              (const char*)librdf_uri_as_string(plugin_b->plugin_uri));
}
*/

void
slv2_world_load_specifications(SLV2World world)
{
	unsigned char* query_string = (unsigned char*)
    	"PREFIX : <http://lv2plug.in/ns/lv2core#>\n"
		"PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>\n"
		"SELECT DISTINCT ?spec ?data WHERE {\n"
		"	?spec a            :Specification ;\n"
		"         rdfs:seeAlso ?data .\n"
		"}\n";
	
	librdf_query* q = librdf_new_query(world->world, "sparql", NULL, query_string, NULL);
	
	librdf_query_results* results = librdf_query_execute(q, world->model);

	while (!librdf_query_results_finished(results)) {
		librdf_node* spec_node = librdf_query_results_get_binding_value(results, 0);
		//librdf_uri*  spec_uri  = librdf_node_get_uri(spec_node);
		librdf_node* data_node = librdf_query_results_get_binding_value(results, 1);
		librdf_uri*  data_uri  = librdf_node_get_uri(data_node);

		slv2_world_load_file(world, data_uri);

		librdf_free_node(spec_node);
		librdf_free_node(data_node);

		librdf_query_results_next(results);
	}

	librdf_free_query_results(results);
	librdf_free_query(q);
}


void
slv2_world_load_plugin_classes(SLV2World world)
{
	// FIXME: This will need to be a bit more clever when more data is around
	// then the ontology (ie classes which aren't LV2 plugin_classes)
	
	// FIXME: This loads things that aren't plugin categories
	
	unsigned char* query_string = (unsigned char*)
    	"PREFIX : <http://lv2plug.in/ns/lv2core#>\n"
		"PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>\n"
		"SELECT DISTINCT ?class ?parent ?label WHERE {\n"
		//"	?plugin a ?class .\n"
		"	?class a rdfs:Class; rdfs:subClassOf ?parent; rdfs:label ?label\n"
		"} ORDER BY ?class\n";
	
	librdf_query* q = librdf_new_query(world->world, "sparql",
		NULL, query_string, NULL);
	
	librdf_query_results* results = librdf_query_execute(q, world->model);

	while (!librdf_query_results_finished(results)) {
		librdf_node* class_node  = librdf_query_results_get_binding_value(results, 0);
		librdf_uri*  class_uri   = librdf_node_get_uri(class_node);
		librdf_node* parent_node = librdf_query_results_get_binding_value(results, 1);
		librdf_uri*  parent_uri  = librdf_node_get_uri(parent_node);
		librdf_node* label_node  = librdf_query_results_get_binding_value(results, 2);
		const char*  label       = (const char*)librdf_node_get_literal_value(label_node);

		SLV2PluginClass plugin_class = slv2_plugin_class_new(world,
				(const char*)librdf_uri_as_string(parent_uri),
				(const char*)librdf_uri_as_string(class_uri),
				label);
		raptor_sequence_push(world->plugin_classes, plugin_class);

		librdf_free_node(class_node);
		librdf_free_node(parent_node);
		librdf_free_node(label_node);

		librdf_query_results_next(results);
	}

	// FIXME: filter list here
	
	librdf_free_query_results(results);
	librdf_free_query(q);
}


void
slv2_world_load_all(SLV2World world)
{
	char* lv2_path = getenv("LV2_PATH");

	/* 1. Read LV2 ontology into model */
	const char* ontology_path = "/usr/local/share/slv2/lv2.ttl";
	FILE* ontology = fopen(ontology_path, "r");
	if (ontology == NULL) {
		ontology_path = "/usr/share/slv2/lv2.ttl";
		ontology = fopen(ontology_path, "r");
	}

	if (ontology) {
		fclose(ontology);
		librdf_uri* ontology_uri = librdf_new_uri_from_filename(world->world,
				ontology_path);
		librdf_parser_parse_into_model(world->parser, ontology_uri, NULL, world->model);
		librdf_free_uri(ontology_uri);
	}

	/* 2. Read all manifest files into model */

	if (lv2_path) {
		slv2_world_load_path(world, lv2_path);
	} else {
		const char* const home = getenv("HOME");
		if (home) {
			const char* const suffix = "/.lv2:/usr/local/lib/lv2:/usr/lib/lv2";
			lv2_path = slv2_strjoin(home, suffix, NULL);
		} else {
			lv2_path = strdup("/usr/local/lib/lv2:/usr/lib/lv2");
		}

		slv2_world_load_path(world, lv2_path);

		free(lv2_path);
	}

	
	/* 3. Query out things to cache */

	slv2_world_load_specifications(world);

	slv2_world_load_plugin_classes(world);

	// Find all plugins and associated data files
	unsigned char* query_string = (unsigned char*)
    	"PREFIX : <http://lv2plug.in/ns/lv2core#>\n"
		"PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>\n"
		"PREFIX slv2: <http://drobilla.net/ns/slv2#>\n"
		"SELECT DISTINCT ?plugin ?data ?bundle ?binary\n"
		"WHERE { ?plugin a :Plugin; slv2:bundleURI ?bundle; rdfs:seeAlso ?data\n"
		"OPTIONAL { ?plugin :binary ?binary } }\n"
		"ORDER BY ?plugin\n";
	
	librdf_query* q = librdf_new_query(world->world, "sparql",
		NULL, query_string, NULL);
	
	librdf_query_results* results = librdf_query_execute(q, world->model);

	while (!librdf_query_results_finished(results)) {
	
		librdf_node* plugin_node = librdf_query_results_get_binding_value(results, 0);
		librdf_uri*  plugin_uri  = librdf_node_get_uri(plugin_node);
		librdf_node* data_node   = librdf_query_results_get_binding_value(results, 1);
		librdf_uri*  data_uri    = librdf_node_get_uri(data_node);
		librdf_node* bundle_node = librdf_query_results_get_binding_value(results, 2);
		librdf_uri*  bundle_uri  = librdf_node_get_uri(bundle_node);
		librdf_node* binary_node = librdf_query_results_get_binding_value(results, 3);
		librdf_uri*  binary_uri  = librdf_node_get_uri(binary_node);
		
		SLV2Plugin plugin = slv2_plugins_get_by_uri(world->plugins,
				(const char*)librdf_uri_as_string(plugin_uri));
		
		// Create a new SLV2Plugin
		if (!plugin) {
			plugin = slv2_plugin_new(world, plugin_uri, bundle_uri, binary_uri);
			raptor_sequence_push(world->plugins, plugin);
		}
		
		plugin->world = world;

		// FIXME: check for duplicates
		raptor_sequence_push(plugin->data_uris,
				slv2_value_new(SLV2_VALUE_URI, (const char*)librdf_uri_as_string(data_uri)));
		
		librdf_free_node(plugin_node);
		librdf_free_node(data_node);
		librdf_free_node(bundle_node);
		librdf_free_node(binary_node);

		librdf_query_results_next(results);
	}

	// 'ORDER BY ?plugin' guarantees this
	//raptor_sequence_sort(world->plugins, slv2_plugin_compare_by_uri);

	if (results)
		librdf_free_query_results(results);
	
	librdf_free_query(q);
}


#if 0
void
slv2_world_serialize(const char* filename)
{
	librdf_uri* lv2_uri = librdf_new_uri(slv2_rdf_world,
			(unsigned char*)"http://lv2plug.in/ns/lv2core#");
	
	librdf_uri* rdfs_uri = librdf_new_uri(slv2_rdf_world,
			(unsigned char*)"http://www.w3.org/2000/01/rdf-schema#");

	// Write out test file
	librdf_serializer* serializer = librdf_new_serializer(slv2_rdf_world,
			"turtle", NULL, NULL);
	librdf_serializer_set_namespace(serializer, lv2_uri, "");
	librdf_serializer_set_namespace(serializer, rdfs_uri, "rdfs");
	librdf_serializer_serialize_world_to_file(serializer, filename, NULL, slv2_model);
	librdf_free_serializer(serializer);
}
#endif


SLV2PluginClass
slv2_world_get_plugin_class(SLV2World world)
{
	return raptor_sequence_get_at(world->plugin_classes, 0);
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


#if 0
SLV2Plugins
slv2_world_get_plugins_by_query(SLV2World world, const char* query)
{
	SLV2Plugins list = slv2_plugins_new();	

	librdf_query* rq = librdf_new_query(world->world, "sparql",
		NULL, (const unsigned char*)query, NULL);
	
	librdf_query_results* results = librdf_query_execute(rq, world->model);
	
	while (!librdf_query_results_finished(results)) {
		librdf_node* plugin_node = librdf_query_results_get_binding_value(results, 0);
		librdf_uri*  plugin_uri  = librdf_node_get_uri(plugin_node);
		
		SLV2Plugin plugin = slv2_plugins_get_by_uri(list,
				(const char*)librdf_uri_as_string(plugin_uri));

		/* Create a new SLV2Plugin */
		if (!plugin) {
			SLV2Plugin new_plugin = slv2_plugin_new(world, plugin_uri);
			raptor_sequence_push(list, new_plugin);
		}
		
		librdf_free_node(plugin_node);
		
		librdf_query_results_next(results);
	}	
	
	if (results)
		librdf_free_query_results(results);
	
	librdf_free_query(rq);

	return list;
}
#endif

