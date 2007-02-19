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

#define _XOPEN_SOURCE 500
#include <rasqal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <dirent.h>
#include <slv2/types.h>
#include <slv2/plugin.h>
#include <slv2/pluginlist.h>
#include <slv2/stringlist.h>
#include <slv2/util.h>
#include "private_types.h"


/* not exposed */
struct _Plugin*
slv2_plugin_new()
{
	struct _Plugin* result = malloc(sizeof(struct _Plugin));
	result->plugin_uri = NULL;
	result->bundle_url = NULL;
	result->lib_uri    = NULL;
	
	result->data_uris = slv2_strings_new();

	return result;
}

	
struct _PluginList*
slv2_plugins_new()
{
	struct _PluginList* result = malloc(sizeof(struct _PluginList));
	result->num_plugins = 0;
	result->plugins = NULL;
	return result;
}


void
slv2_plugins_free(SLV2Plugins list)
{
	list->num_plugins = 0;
	free(list->plugins);
	free(list);
}


void
slv2_plugins_load_all(SLV2Plugins list)
{
	assert(list != NULL);
	
	char* slv2_path = getenv("LV2_PATH");

	if (slv2_path) {
	    slv2_plugins_load_path(list, slv2_path);
    } else {
        const char* const home = getenv("HOME");
        const char* const suffix = "/.lv2:/usr/local/lib/lv2:usr/lib/lv2";
        slv2_path = slv2_strjoin(home, suffix, NULL);
		
		fprintf(stderr, "$LV2_PATH is unset.  Using default path %s\n", slv2_path);
	    
		/* pass 1: find all plugins */
		slv2_plugins_load_path(list, slv2_path);
		
		/* pass 2: find all data files for plugins */
		slv2_plugins_load_path(list, slv2_path);

        free(slv2_path);
	}
}


/* This is the parser for manifest.ttl
 * This is called twice on each bundle in the discovery process, which is (much) less
 * efficient than it could be.... */
void
slv2_plugins_load_bundle(SLV2Plugins    list,
                      const char* bundle_base_url)
{
	unsigned char* manifest_url = malloc(
		(strlen((char*)bundle_base_url) + strlen("manifest.ttl") + 2) * sizeof(unsigned char));
	memcpy(manifest_url, bundle_base_url, strlen((char*)bundle_base_url)+1 * sizeof(unsigned char));
	if (bundle_base_url[strlen(bundle_base_url)-1] == '/')
		strcat((char*)manifest_url, "manifest.ttl");
	else
		strcat((char*)manifest_url, "/manifest.ttl");
	
	rasqal_query_results *results;
	raptor_uri *base_url = raptor_new_uri(manifest_url);
	rasqal_query *rq = rasqal_new_query("sparql", NULL);

	/* Get all plugins explicitly mentioned in the manifest (discovery pass 1) */
	char* query_string =
    	"PREFIX : <http://lv2plug.in/ontology#>\n\n"
		"SELECT DISTINCT ?plugin_uri FROM <>\n"
		"WHERE { ?plugin_uri a :Plugin }\n";

	//printf("%s\n\n", query_string);  
	
	rasqal_query_prepare(rq, (unsigned char*)query_string, base_url);
	results = rasqal_query_execute(rq);
	
	while (!rasqal_query_results_finished(results)) {
		
		rasqal_literal* literal = rasqal_query_results_get_binding_value(results, 0);
		assert(literal);

		if (!slv2_plugins_get_by_uri(list, (const char*)rasqal_literal_as_string(literal))) {
			/* Create a new plugin */
			struct _Plugin* new_plugin = slv2_plugin_new();
			new_plugin->plugin_uri = strdup((const char*)rasqal_literal_as_string(literal));
			new_plugin->bundle_url = strdup(bundle_base_url);
			raptor_sequence_push(new_plugin->data_uris, strdup((const char*)manifest_url));

			/* And add it to the list
			 * Yes, this is disgusting, but it doesn't seem there's a way to know
			 * how many matches there are before iterating over them.. */
			list->num_plugins++;
			list->plugins = realloc(list->plugins,
				list->num_plugins * sizeof(struct _Plugin*));
			list->plugins[list->num_plugins-1] = new_plugin;

		}
		
		rasqal_query_results_next(results);
	}	
	
	if (results)
		rasqal_free_query_results(results);
	
	rasqal_free_query(rq);
	
	rq = rasqal_new_query("sparql", NULL);
	
	/* Get all data files linked to plugins (discovery pass 2) */
	query_string =
		"PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>\n"
    	"PREFIX :     <http://lv2plug.in/ontology#>\n\n"
    	"SELECT DISTINCT ?subject ?data_uri ?binary FROM <>\n"
    	"WHERE { ?subject  rdfs:seeAlso  ?data_uri\n"
		"OPTIONAL { ?subject :binary ?binary } }\n";

	//printf("%s\n\n", query_string);  
	
	rasqal_query_prepare(rq, (unsigned char*)query_string, base_url);
	results = rasqal_query_execute(rq);

	while (!rasqal_query_results_finished(results)) {

		const char* subject = (const char*)rasqal_literal_as_string(
				rasqal_query_results_get_binding_value(results, 0));

		const char* data_uri = (const char*)rasqal_literal_as_string(
				rasqal_query_results_get_binding_value(results, 1));

		const char* binary = (const char*)rasqal_literal_as_string(
				rasqal_query_results_get_binding_value(results, 2));

		SLV2Plugin plugin = slv2_plugins_get_by_uri(list, subject);

		if (plugin && data_uri && !slv2_strings_contains(plugin->data_uris, data_uri))
			raptor_sequence_push(plugin->data_uris, strdup(data_uri));
		
		if (plugin && binary && !plugin->lib_uri)
			((struct _Plugin*)plugin)->lib_uri = strdup(binary);
		 
		rasqal_query_results_next(results);

	}

	if (results)
		rasqal_free_query_results(results);
	
	rasqal_free_query(rq);
		

	raptor_free_uri(base_url);
	free(manifest_url);
}


/* Add all the plugins found in dir to list.
 * (Private helper function, not exposed in public API)
 */
void
slv2_plugins_load_dir(SLV2Plugins list, const char* dir)
{
	DIR* pdir = opendir(dir);
	if (!pdir)
		return;
	
	struct dirent* pfile;
	while ((pfile = readdir(pdir))) {
		if (!strcmp(pfile->d_name, ".") || !strcmp(pfile->d_name, ".."))
			continue;

		char* bundle_path = slv2_strjoin(dir, "/", pfile->d_name, NULL);
		char* bundle_url = slv2_strjoin("file://", dir, "/", pfile->d_name, NULL);
		DIR* bundle_dir = opendir(bundle_path);

		if (bundle_dir != NULL) {
			closedir(bundle_dir);
			
			slv2_plugins_load_bundle(list, bundle_url);
			//printf("Loaded bundle %s\n", bundle_url);
		}
		
		free(bundle_path);
	}

	closedir(pdir);
}


void
slv2_plugins_load_path(SLV2Plugins    list,
                    const char* lv2_path)
{
	char* path = slv2_strjoin(lv2_path, ":", NULL);

	char* dir = path; // Pointer into path
	
	// Go through string replacing ':' with '\0', using the substring,
	// then replacing it with 'X' and moving on.  i.e. strtok on crack.
	while (strchr(path, ':') != NULL) {
		char* delim = strchr(path, ':');
		*delim = '\0';
		
		slv2_plugins_load_dir(list, dir);
		
		*delim = 'X';
		dir = delim + 1;
	}
	
	//char* slv2_path = strdup(slv2

	free(path);
}


unsigned
slv2_plugins_size(const SLV2Plugins list)
{
	assert(list != NULL);
	return list->num_plugins;
}


SLV2Plugin
slv2_plugins_get_by_uri(const SLV2Plugins list, const char* uri)
{
	if (list->num_plugins > 0) {	
		assert(list->plugins != NULL);
		
		for (unsigned i=0; i < list->num_plugins; ++i)
			if (!strcmp((char*)list->plugins[i]->plugin_uri, (char*)uri))
				return list->plugins[i];
	}

	return NULL;
}


SLV2Plugin
slv2_plugins_get_at(const SLV2Plugins list, unsigned index)
{
	if (list->num_plugins == 0)
		return NULL;
	
	assert(list->plugins != NULL);
	
	return (index < list->num_plugins) ? list->plugins[index] : NULL;
}

