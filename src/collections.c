/*
  Copyright 2008-2011 David Robillard <http://drobilla.net>

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

#include <glib.h>

#include "slv2_internal.h"

/* SEQUENCE */

#define SLV2_SEQUENCE_IMPL(CollType, ElemType, prefix, free_func) \
\
CollType \
prefix ## _new() \
{ \
	return g_sequence_new((GDestroyNotify)free_func); \
} \
\
SLV2_API \
void \
prefix ## _free(CollType coll) \
{ \
	if (coll) \
		g_sequence_free((GSequence*)coll); \
} \
\
SLV2_API \
unsigned \
prefix ## _size(CollType coll) \
{ \
	return (coll ? g_sequence_get_length((GSequence*)coll) : 0); \
} \
\
SLV2_API \
ElemType \
prefix ## _get_at(CollType coll, unsigned index) \
{ \
	if (!coll || index >= prefix ## _size(coll)) { \
		return NULL; \
	} else { \
		GSequenceIter* i = g_sequence_get_iter_at_pos((GSequence*)coll, (int)index); \
		return (ElemType)g_sequence_get(i); \
	} \
}

SLV2_SEQUENCE_IMPL(SLV2ScalePoints, SLV2ScalePoint,
                   slv2_scale_points, &slv2_scale_point_free)

SLV2_SEQUENCE_IMPL(SLV2Values, SLV2Value,
                   slv2_values, &slv2_value_free)

SLV2_SEQUENCE_IMPL(SLV2PluginClasses, SLV2PluginClass,
                   slv2_plugin_classes, &slv2_plugin_class_free)

SLV2_SEQUENCE_IMPL(SLV2UIs, SLV2UI,
                   slv2_uis, &slv2_ui_free)

SLV2_API
SLV2PluginClass
slv2_plugin_classes_get_by_uri(SLV2PluginClasses coll, SLV2Value uri)
{
	return (SLV2PluginClass)slv2_sequence_get_by_uri(coll, uri);
}

SLV2_API
SLV2UI
slv2_uis_get_by_uri(SLV2UIs coll, SLV2Value uri)
{
	return (SLV2UIs)slv2_sequence_get_by_uri(coll, uri);
}


/* VALUES */

SLV2_API
bool
slv2_values_contains(SLV2Values list, SLV2Value value)
{
	for (unsigned i = 0; i < slv2_values_size(list); ++i)
		if (slv2_value_equals(slv2_values_get_at(list, i), value))
			return true;

	return false;
}

