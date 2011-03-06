/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGE.
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
