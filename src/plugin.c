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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <rasqal.h>
#include <slv2/plugin.h>
#include <slv2/types.h>
#include <slv2/util.h>
#include <slv2/stringlist.h>
#include "private_types.h"


SLV2Plugin
slv2_plugin_duplicate(SLV2Plugin p)
{
	assert(p);
	struct _Plugin* result = malloc(sizeof(struct _Plugin));
	result->plugin_uri = p->plugin_uri;
	result->bundle_url = p->bundle_url;
	result->lib_uri = p->lib_uri;
	
	result->data_uris = slv2_strings_new();
	for (unsigned i=0; i < slv2_strings_size(p->data_uris); ++i)
		raptor_sequence_push(result->data_uris, strdup(slv2_strings_get_at(p->data_uris, i)));
	return result;
}


const char*
slv2_plugin_get_uri(SLV2Plugin p)
{
	assert(p);
	return p->plugin_uri;
}


SLV2Strings
slv2_plugin_get_data_uris(SLV2Plugin p)
{
	assert(p);
	return p->data_uris;
}


const char*
slv2_plugin_get_library_uri(SLV2Plugin p)
{
	assert(p);
	return p->lib_uri;
}


bool
slv2_plugin_verify(SLV2Plugin plugin)
{
	// FIXME: finish this (properly)
	
	size_t num_values = 0;
	
	SLV2Strings prop = slv2_plugin_get_value(plugin, "doap:name");
	if (prop) {
		num_values = slv2_strings_size(prop);
		slv2_strings_free(prop);
	}
	if (num_values < 1)
		return false;
/*
	prop = slv2_plugin_get_value(plugin, "doap:license");
	num_values = prop->num_values;
	free(prop);
	if (num_values < 1)
		return false;
*/	
	return true;
}


char*
slv2_plugin_get_name(SLV2Plugin plugin)
{
	char*          result = NULL;
	SLV2Strings prop   = slv2_plugin_get_value(plugin, "doap:name");
	
	// FIXME: guaranteed to be the untagged one?
	if (prop && slv2_strings_size(prop) >= 1)
		result = strdup(slv2_strings_get_at(prop, 0));

	if (prop)
		slv2_strings_free(prop);

	return result;
}


SLV2Strings
slv2_plugin_get_value(SLV2Plugin  p,
                      const char* predicate)
{
	assert(predicate);

    char* query = slv2_strjoin(
		"SELECT DISTINCT ?value WHERE {\n"
		"plugin: ", predicate, " ?value .\n"
		"}\n", NULL);

	SLV2Strings result = slv2_plugin_simple_query(p, query, "value");
	
	free(query);

	return result;
}


SLV2Strings
slv2_plugin_get_properties(SLV2Plugin p)
{
	return slv2_plugin_get_value(p, "lv2:pluginProperty");
}


SLV2Strings
slv2_plugin_get_hints(SLV2Plugin p)
{
	return slv2_plugin_get_value(p, "lv2:pluginHint");
}


uint32_t
slv2_plugin_get_num_ports(SLV2Plugin p)
{
    const char* const query =
		"SELECT DISTINCT ?port\n"
		"WHERE { plugin: lv2:port ?port }\n";

	return (uint32_t)slv2_plugin_query_count(p, query);
}


bool
slv2_plugin_has_latency(SLV2Plugin p)
{
    const char* const query = 
		"SELECT DISTINCT ?port WHERE {\n"
		"	plugin: lv2:port     ?port .\n"
		"	?port   lv2:portHint lv2:reportsLatency .\n"
		"}\n";

	SLV2Strings results = slv2_plugin_simple_query(p, query, "port");
	
	bool exists = (slv2_strings_size(results) > 0);
	
	slv2_strings_free(results);

	return exists;
}


uint32_t
slv2_plugin_get_latency_port(SLV2Plugin p)
{
    const char* const query = 
		"SELECT DISTINCT ?value WHERE {\n"
		"	plugin: lv2:port     ?port .\n"
		"	?port   lv2:portHint lv2:reportsLatency ;\n"
		"           lv2:index    ?index .\n"
		"}\n";

	SLV2Strings result = slv2_plugin_simple_query(p, query, "index");
	
	// FIXME: need a sane error handling strategy
	assert(slv2_strings_size(result) == 1);
	char* endptr = 0;
	uint32_t index = strtol(slv2_strings_get_at(result, 0), &endptr, 10);
	// FIXME: check.. stuff..

	return index;
}


SLV2Strings
slv2_plugin_get_supported_features(SLV2Plugin p)
{
    const char* const query = 
		"SELECT DISTINCT ?feature WHERE {\n"
		"	{ plugin:  lv2:optionalHostFeature  ?feature }\n"
		"		UNION\n"
		"	{ plugin:  lv2:requiredHostFeature  ?feature }\n"
		"}\n";

	SLV2Strings result = slv2_plugin_simple_query(p, query, "feature");
	
	return result;
}


SLV2Strings
slv2_plugin_get_optional_features(SLV2Plugin p)
{
    const char* const query = 
		"SELECT DISTINCT ?feature WHERE {\n"
		"	plugin:  lv2:optionalHostFeature  ?feature .\n"
		"}\n";

	SLV2Strings result = slv2_plugin_simple_query(p, query, "feature");
	
	return result;
}


SLV2Strings
slv2_plugin_get_required_features(SLV2Plugin p)
{
    const char* const query = 
		"SELECT DISTINCT ?feature WHERE {\n"
		"	plugin:  lv2:requiredHostFeature  ?feature .\n"
		"}\n";

	SLV2Strings result = slv2_plugin_simple_query(p, query, "feature");
	
	return result;
}

