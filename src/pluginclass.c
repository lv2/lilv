/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "slv2_internal.h"

SLV2PluginClass
slv2_plugin_class_new(SLV2World   world,
                      SLV2Node    parent_node,
                      SLV2Node    uri,
                      const char* label)
{
	if (parent_node && sord_node_get_type(parent_node) != SORD_URI) {
		return NULL;  // Not an LV2 plugin superclass (FIXME: discover properly)
	}
	SLV2PluginClass pc = (SLV2PluginClass)malloc(sizeof(struct _SLV2PluginClass));
	pc->world      = world;
	pc->uri        = slv2_value_new_from_node(world, uri);
	pc->label      = slv2_value_new(world, SLV2_VALUE_STRING, label);
	pc->parent_uri = (parent_node)
		? slv2_value_new_from_node(world, parent_node)
		: NULL;
	return pc;
}

void
slv2_plugin_class_free(SLV2PluginClass plugin_class)
{
	assert(plugin_class->uri);
	slv2_value_free(plugin_class->uri);
	slv2_value_free(plugin_class->parent_uri);
	slv2_value_free(plugin_class->label);
	free(plugin_class);
}

SLV2_API
SLV2Value
slv2_plugin_class_get_parent_uri(SLV2PluginClass plugin_class)
{
	if (plugin_class->parent_uri)
		return plugin_class->parent_uri;
	else
		return NULL;
}

SLV2_API
SLV2Value
slv2_plugin_class_get_uri(SLV2PluginClass plugin_class)
{
	assert(plugin_class->uri);
	return plugin_class->uri;
}

SLV2_API
SLV2Value
slv2_plugin_class_get_label(SLV2PluginClass plugin_class)
{
	return plugin_class->label;
}

SLV2_API
SLV2PluginClasses
slv2_plugin_class_get_children(SLV2PluginClass plugin_class)
{
	// Returned list doesn't own categories
	SLV2PluginClasses all    = plugin_class->world->plugin_classes;
	SLV2PluginClasses result = g_sequence_new(NULL);

	for (GSequenceIter* i = g_sequence_get_begin_iter(all);
	     i != g_sequence_get_end_iter(all);
	     i = g_sequence_iter_next(i)) {
		SLV2PluginClass c      = g_sequence_get(i);
		SLV2Value       parent = slv2_plugin_class_get_parent_uri(c);
		if (parent && slv2_value_equals(slv2_plugin_class_get_uri(plugin_class), parent))
			slv2_sequence_insert(result, c);
	}

	return result;
}
