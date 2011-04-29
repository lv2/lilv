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

#include "lilv_internal.h"

LilvPluginClass*
lilv_plugin_class_new(LilvWorld*      world,
                      const SordNode* parent_node,
                      const SordNode* uri,
                      const char*     label)
{
	if (parent_node && sord_node_get_type(parent_node) != SORD_URI) {
		return NULL;  // Not an LV2 plugin superclass (FIXME: discover properly)
	}
	LilvPluginClass* pc = malloc(sizeof(struct LilvPluginClassImpl));
	pc->world      = world;
	pc->uri        = lilv_value_new_from_node(world, uri);
	pc->label      = lilv_value_new(world, LILV_VALUE_STRING, label);
	pc->parent_uri = (parent_node)
		? lilv_value_new_from_node(world, parent_node)
		: NULL;
	return pc;
}

void
lilv_plugin_class_free(LilvPluginClass* plugin_class)
{
	assert(plugin_class->uri);
	lilv_value_free(plugin_class->uri);
	lilv_value_free(plugin_class->parent_uri);
	lilv_value_free(plugin_class->label);
	free(plugin_class);
}

LILV_API
const LilvValue*
lilv_plugin_class_get_parent_uri(const LilvPluginClass* plugin_class)
{
	if (plugin_class->parent_uri)
		return plugin_class->parent_uri;
	else
		return NULL;
}

LILV_API
const LilvValue*
lilv_plugin_class_get_uri(const LilvPluginClass* plugin_class)
{
	assert(plugin_class->uri);
	return plugin_class->uri;
}

LILV_API
const LilvValue*
lilv_plugin_class_get_label(const LilvPluginClass* plugin_class)
{
	return plugin_class->label;
}

LILV_API
LilvPluginClasses*
lilv_plugin_class_get_children(const LilvPluginClass* plugin_class)
{
	// Returned list doesn't own categories
	LilvPluginClasses* all    = plugin_class->world->plugin_classes;
	LilvPluginClasses* result = g_sequence_new(NULL);

	for (GSequenceIter* i = g_sequence_get_begin_iter(all);
	     i != g_sequence_get_end_iter(all);
	     i = g_sequence_iter_next(i)) {
		const LilvPluginClass* c      = g_sequence_get(i);
		const LilvValue*       parent = lilv_plugin_class_get_parent_uri(c);
		if (parent && lilv_value_equals(lilv_plugin_class_get_uri(plugin_class),
		                                parent))
			lilv_sequence_insert(result, (LilvPluginClass*)c);
	}

	return result;
}
