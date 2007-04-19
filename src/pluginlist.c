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
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <dirent.h>
#include <librdf.h>
#include <slv2/types.h>
#include <slv2/plugin.h>
#include <slv2/pluginlist.h>
#include <slv2/stringlist.h>
#include <slv2/util.h>
#include "private_types.h"

	
SLV2Plugins
slv2_plugins_new()
{
	return raptor_new_sequence((void (*)(void*))&slv2_plugin_free, NULL);
}


void
slv2_plugins_free(SLV2Plugins list)
{
	raptor_free_sequence(list);
}

#if 0
void
slv2_plugins_filter(SLV2Plugins dest, SLV2Plugins source, bool (*include)(SLV2Plugin))
{
	assert(dest);

	for (int i=0; i < raptor_sequence_size(source); ++i) {
		SLV2Plugin p = raptor_sequence_get_at(source, i);
		if (include(p))
			raptor_sequence_push(dest, slv2_plugin_duplicate(p));
	}
}


void
slv2_plugins_load_all(SLV2Plugins list)
{
	/* FIXME: this is much slower than it should be in many ways.. */

	assert(list);
	
	char* slv2_path = getenv("LV2_PATH");

	SLV2Plugins load_list = slv2_plugins_new();	

	if (slv2_path) {
	    slv2_plugins_load_path(load_list, slv2_path);
    } else {
        const char* const home = getenv("HOME");
        const char* const suffix = "/.lv2:/usr/local/lib/lv2:usr/lib/lv2";
        slv2_path = slv2_strjoin(home, suffix, NULL);
		
		fprintf(stderr, "$LV2_PATH is unset.  Using default path %s\n", slv2_path);
	    
		/* pass 1: find all plugins */
		slv2_plugins_load_path(load_list, slv2_path);
		
		/* pass 2: find all data files for plugins */
		slv2_plugins_load_path(load_list, slv2_path);

        free(slv2_path);
	}

	/* insert only valid plugins into list */
	slv2_plugins_filter(list, load_list, slv2_plugin_verify);
	slv2_plugins_free(load_list);
}


/* This is the parser for manifest.ttl
 * This is called twice on each bundle in the discovery process, which is (much) less
 * efficient than it could be.... */
void
slv2_plugins_load_bundle(SLV2Plugins list,
                         const char* bundle_base_url)
{
	assert(list);

	unsigned char* manifest_url = malloc(
		(strlen((char*)bundle_base_url) + strlen("manifest.ttl") + 2) * sizeof(unsigned char));
	memcpy(manifest_url, bundle_base_url, strlen((char*)bundle_base_url)+1 * sizeof(unsigned char));
	if (bundle_base_url[strlen(bundle_base_url)-1] == '/')
		strcat((char*)manifest_url, "manifest.ttl");
	else
		strcat((char*)manifest_url, "/manifest.ttl");
	
	librdf_query_results *results;
	librdf_uri *base_uri = librdf_new_uri(slv2_rdf_world, manifest_url);
	
	/* Get all plugins explicitly mentioned in the manifest (discovery pass 1) */
	char* query_string =
    	"PREFIX : <http://lv2plug.in/ontology#>\n\n"
		"SELECT DISTINCT ?plugin_uri FROM <>\n"
		"WHERE { ?plugin_uri a :Plugin }\n";
	
	librdf_query *rq = librdf_new_query(slv2_rdf_world, "sparql", NULL,
		(unsigned char*)query_string, base_uri);



	//printf("%s\n\n", query_string);  
	
	results = librdf_query_execute(rq, model->model);
	
	while (!librdf_query_results_finished(results)) {
		
		librdf_node* literal = librdf_query_results_get_binding_value(results, 0);
		assert(literal);

		if (!slv2_plugins_get_by_uri(list, (const char*)librdf_node_get_literal_value(literal))) {
			/* Create a new plugin */
			struct _Plugin* new_plugin = slv2_plugin_new();
			new_plugin->plugin_uri = strdup((const char*)librdf_node_get_literal_value(literal));
			new_plugin->bundle_url = strdup(bundle_base_url);
			raptor_sequence_push(new_plugin->data_uris, strdup((const char*)manifest_url));

			raptor_sequence_push(list, new_plugin);

		}
		
		librdf_query_results_next(results);
	}	
	
	if (results)
		librdf_free_query_results(results);
	
	librdf_free_query(rq);
	
	/* Get all data files linked to plugins (discovery pass 2) */
	query_string =
		"PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>\n"
    	"PREFIX :     <http://lv2plug.in/ontology#>\n\n"
    	"SELECT DISTINCT ?subject ?data_uri ?binary FROM <>\n"
    	"WHERE { ?subject  rdfs:seeAlso  ?data_uri\n"
		"OPTIONAL { ?subject :binary ?binary } }\n";
	
	rq = librdf_new_query(slv2_rdf_world, "sparql", NULL,
			(unsigned char*)query_string, base_uri);

	//printf("%s\n\n", query_string);  
	
	results = librdf_query_execute(rq, slv2_model);

	while (!librdf_query_results_finished(results)) {

		const char* subject = (const char*)librdf_node_get_literal_value(
				librdf_query_results_get_binding_value(results, 0));

		const char* data_uri = (const char*)librdf_node_get_literal_value(
				librdf_query_results_get_binding_value(results, 1));

		const char* binary = (const char*)librdf_node_get_literal_value(
				librdf_query_results_get_binding_value(results, 2));

		SLV2Plugin plugin = slv2_plugins_get_by_uri(list, subject);

		if (plugin && data_uri && !slv2_strings_contains(plugin->data_uris, data_uri))
			raptor_sequence_push(plugin->data_uris, strdup(data_uri));
		
		if (plugin && binary && !plugin->lib_uri)
			((struct _Plugin*)plugin)->lib_uri = strdup(binary);
		 
		librdf_query_results_next(results);

	}

	if (results)
		librdf_free_query_results(results);
	
	librdf_free_query(rq);

	librdf_free_uri(base_uri);
	free(manifest_url);
}


/* Add all the plugins found in dir to list.
 * (Private helper function, not exposed in public API)
 */
void
slv2_plugins_load_dir(SLV2Plugins list, const char* dir)
{
	assert(list);

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
		free(bundle_url);
	}

	closedir(pdir);
}


void
slv2_plugins_load_path(SLV2Plugins list,
                       const char* lv2_path)
{
	assert(list);

	char* path = slv2_strjoin(lv2_path, ":", NULL);
	char* dir  = path; // Pointer into path
	
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
#endif

unsigned
slv2_plugins_size(SLV2Plugins list)
{
	return raptor_sequence_size(list);
}


SLV2Plugin
slv2_plugins_get_by_uri(SLV2Plugins list, const char* uri)
{
	// good old fashioned binary search
	
	int lower = 0;
	int upper = raptor_sequence_size(list) - 1;
	int i;
	
	if (upper == 0)
		return NULL;

	while (upper >= lower) {
		i = lower + ((upper - lower) / 2);

		SLV2Plugin p = raptor_sequence_get_at(list, i);

		int cmp = strcmp(slv2_plugin_get_uri(p), uri);

		if (cmp == 0)
			return p;
		else if (cmp > 0)
			upper = i - 1;
		else
			lower = i + 1;
	}

	return NULL;
}


SLV2Plugin
slv2_plugins_get_at(SLV2Plugins list, unsigned index)
{	
	assert(list);

	if (index > INT_MAX)
		return NULL;
	else
		return (SLV2Plugin)raptor_sequence_get_at(list, (int)index);
}

