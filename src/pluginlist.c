/* LibSLV2
 * Copyright (C) 2006 Dave Robillard <drobilla@connect.carleton.ca>
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
#include <slv2/private_types.h>
#include <slv2/types.h>
#include <slv2/plugin.h>
#include <slv2/pluginlist.h>
#include "util.h"


struct _PluginList*
slv2_list_new()
{
	struct _PluginList* result = malloc(sizeof(struct _PluginList));
	result->num_plugins = 0;
	result->plugins = NULL;
	return result;
}


void
slv2_list_free(SLV2List list)
{
	list->num_plugins = 0;
	free(list->plugins);
	free(list);
}


void
slv2_list_load_all(SLV2List list)
{
	assert(list != NULL);
	
	const char* slv2_path = getenv("LV2_PATH");

	if (slv2_path) {
	    slv2_list_load_path(list, slv2_path);
    } else {
        const char* const home = getenv("HOME");
        const char* const suffix = "/.lv2:/usr/local/lib/lv2:usr/lib/lv2";
        slv2_path = strjoin(home, suffix);
		
		printf("$LV2_PATH is unset.  Using default path %s\n", slv2_path);
	    slv2_list_load_path(list, slv2_path);

        free(slv2_path);
	}
}


/* This is the parser for manifest.ttl */
void
slv2_list_load_bundle(SLV2List    list,
                      const char* bundle_base_uri)
{
	// FIXME: ew
	unsigned char* manifest_uri = malloc(
		(strlen((char*)bundle_base_uri) + strlen("manifest.ttl") + 2) * sizeof(unsigned char));
	memcpy(manifest_uri, bundle_base_uri, strlen((char*)bundle_base_uri)+1 * sizeof(unsigned char));
	if (bundle_base_uri[strlen((char*)bundle_base_uri)-1] == '/')
		strcat((char*)manifest_uri, (char*)"manifest.ttl");
	else
		strcat((char*)manifest_uri, (char*)"/manifest.ttl");
	
	rasqal_init();
	rasqal_query_results *results;
	raptor_uri *base_uri = raptor_new_uri(manifest_uri);
	rasqal_query *rq = rasqal_new_query("sparql", (unsigned char*)base_uri);

	char* query_string =
	    "PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#> \n"
    	"PREFIX :     <http://lv2plug.in/ontology#> \n\n"
    		 
    	"SELECT DISTINCT $plugin_uri $data_url $lib_url FROM <> WHERE { \n"
    	"$plugin_uri :binary       $lib_url ; \n"
    	"            rdfs:seeAlso  $data_url . \n"
	"} \n";

	//printf("%s\n\n", query_string);  
	
	rasqal_query_prepare(rq, (unsigned char*)query_string, base_uri);
	results = rasqal_query_execute(rq);
	
	while (!rasqal_query_results_finished(results)) {

		// Create a new plugin
		struct _Plugin* new_plugin = malloc(sizeof(struct _Plugin));
		new_plugin->bundle_url = strdup(bundle_base_uri);
		
		rasqal_literal* literal = NULL;
	
		literal = rasqal_query_results_get_binding_value_by_name(results, "plugin_uri");
		if (literal)
			new_plugin->plugin_uri = strdup(rasqal_literal_as_string(literal));
		
		literal = rasqal_query_results_get_binding_value_by_name(results, "data_url");
		if (literal)
			new_plugin->data_url = strdup(rasqal_literal_as_string(literal));
		
		literal = rasqal_query_results_get_binding_value_by_name(results, "lib_url");
		if (literal)
			new_plugin->lib_url = strdup(rasqal_literal_as_string(literal));
		
		/* Add the plugin if it's valid */
		if (new_plugin->lib_url && new_plugin->data_url && new_plugin->plugin_uri
				&& slv2_plugin_verify(new_plugin)) {
			/* Yes, this is disgusting, but it doesn't seem there's a way to know
			 * how many matches there are before iterating over them */
			list->num_plugins++;
			list->plugins = realloc(list->plugins,
				list->num_plugins * sizeof(struct _Plugin*));
			list->plugins[list->num_plugins-1] = new_plugin;
		}

		rasqal_query_results_next(results);
	}

	// FIXME: leaks?  rasqal really doesn't handle missing files well..
	if (results) {
		rasqal_free_query_results(results);
		//rasqal_free_query(rq); // FIXME: crashes?  leak?
		raptor_free_uri(base_uri); // FIXME: leak?
	}
	rasqal_finish();

	free(manifest_uri);
}


/* Add all the plugins found in dir to list.
 * (Private helper function, not exposed in public API)
 */
void
add_plugins_from_dir(SLV2List list, const char* dir)
{
	DIR* pdir = opendir(dir);
	if (!pdir)
		return;
	
	struct dirent* pfile;
	while ((pfile = readdir(pdir))) {
		if (!strcmp(pfile->d_name, ".") || !strcmp(pfile->d_name, ".."))
			continue;

		char* bundle_path = (char*)strjoin(dir, "/", pfile->d_name, NULL);
		char* bundle_url = (char*)strjoin("file://", dir, "/", pfile->d_name, NULL);
		DIR* bundle_dir = opendir(bundle_path);

		if (bundle_dir != NULL) {
			closedir(bundle_dir);
			
			slv2_list_load_bundle(list, bundle_url);
			//printf("Loaded bundle %s\n", bundle_url);
		}
		
		free(bundle_path);
	}

	closedir(pdir);
}


void
slv2_list_load_path(SLV2List  list,
                    const char* slv2_path)
{
	
	char* path = (char*)strjoin(slv2_path, ":", NULL);

	char* dir = path; // Pointer into path
	
	// Go through string replacing ':' with '\0', using the substring,
	// then replacing it with 'X' and moving on.  eg strtok on crack.
	while (strchr(path, ':') != NULL) {
		char* delim = strchr(path, ':');
		*delim = '\0';
		
		add_plugins_from_dir(list, dir);
		
		*delim = 'X';
		dir = delim + 1;
	}
	
	//char* slv2_path = strdup(slv2

	free(path);
}


size_t
slv2_list_get_length(const SLV2List list)
{
	assert(list != NULL);
	return list->num_plugins;
}


SLV2Plugin*
slv2_list_get_plugin_by_uri(const SLV2List list, const char* uri)
{
	if (list->num_plugins > 0) {	
		assert(list->plugins != NULL);
		
		for (size_t i=0; i < list->num_plugins; ++i)
			if (!strcmp((char*)list->plugins[i]->plugin_uri, (char*)uri))
				return list->plugins[i];
	}

	return NULL;
}


SLV2Plugin*
slv2_list_get_plugin_by_index(const SLV2List list, size_t index)
{
	if (list->num_plugins == 0)
		return NULL;
	
	assert(list->plugins != NULL);
	
	return (index < list->num_plugins) ? list->plugins[index] : NULL;
}

