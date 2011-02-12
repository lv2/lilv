/* SLV2
 * Copyright (C) 2008-2011 David Robillard <http://drobilla.net>
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

#include <glib.h>

#include "slv2_internal.h"

/* ARRAYS */

#define SLV2_ARRAY_IMPL(CollType, ElemType, prefix, free_func) \
\
CollType \
prefix ## _new() \
{ \
	return g_ptr_array_new_with_free_func((GDestroyNotify)free_func); \
} \
\
SLV2_API \
void \
prefix ## _free(CollType coll) \
{ \
	if (coll) \
		g_ptr_array_unref((GPtrArray*)coll); \
} \
\
SLV2_API \
unsigned \
prefix ## _size(CollType coll) \
{ \
	return (coll ? ((GPtrArray*)coll)->len : 0); \
} \
\
SLV2_API \
ElemType \
prefix ## _get_at(CollType coll, unsigned index) \
{ \
	if (!coll || index >= ((GPtrArray*)coll)->len) \
		return NULL; \
	else \
		return (ElemType)g_ptr_array_index((GPtrArray*)coll, (int)index); \
}

SLV2_ARRAY_IMPL(SLV2ScalePoints, SLV2ScalePoint,
		slv2_scale_points, &slv2_scale_point_free)
SLV2_ARRAY_IMPL(SLV2Values, SLV2Value,
		slv2_values, &slv2_value_free)


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
} \
\
SLV2_API \
ElemType \
prefix ## _get_by_uri(CollType coll, SLV2Value uri) \
{ \
	return (ElemType)slv2_sequence_get_by_uri(coll, uri); \
}

SLV2_SEQUENCE_IMPL(SLV2PluginClasses, SLV2PluginClass,
                   slv2_plugin_classes, &slv2_plugin_class_free)

#ifdef SLV2_WITH_UI
SLV2_SEQUENCE_IMPL(SLV2UIs, SLV2UI,
                   slv2_uis, &slv2_ui_free)
#endif


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

