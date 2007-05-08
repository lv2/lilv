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

#include <stdlib.h>
#include <string.h>
#include <slv2/pluginclass.h>
#include "slv2_internal.h"


SLV2PluginClass
slv2_plugin_class_new(SLV2World world, const char* parent_uri,  const char* uri, const char* label)
{
	SLV2PluginClass plugin_class = (SLV2PluginClass)malloc(sizeof(struct _SLV2PluginClass));
	plugin_class->world = world;
	if (parent_uri)
		plugin_class->parent_uri = strdup(parent_uri);
	else
		plugin_class->parent_uri = NULL;
	plugin_class->uri = strdup(uri);
	plugin_class->label = strdup(label);
	return plugin_class;
}


void
slv2_plugin_class_free(SLV2PluginClass plugin_class)
{
	free(plugin_class->uri);
	free(plugin_class->parent_uri);
	free(plugin_class->label);
	free(plugin_class);
}


const char*
slv2_plugin_class_get_parent_uri(SLV2PluginClass plugin_class)
{
	return plugin_class->parent_uri;
}


const char*
slv2_plugin_class_get_uri(SLV2PluginClass plugin_class)
{
	return plugin_class->uri;
}


const char*
slv2_plugin_class_get_label(SLV2PluginClass plugin_class)
{
	return plugin_class->label;
}


SLV2PluginClasses
slv2_plugin_class_get_children(SLV2PluginClass plugin_class)
{
	// Returned list doesn't own categories
	SLV2PluginClasses result = raptor_new_sequence(NULL, NULL);

	for (int i=0; i < raptor_sequence_size(plugin_class->world->plugin_classes); ++i) {
		SLV2PluginClass c = raptor_sequence_get_at(plugin_class->world->plugin_classes, i);
		const char* parent = slv2_plugin_class_get_parent_uri(c);
		if (parent && !strcmp(slv2_plugin_class_get_uri(plugin_class), parent))
			raptor_sequence_push(result, c);
	}

	return result;
}
