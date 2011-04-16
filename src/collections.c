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

/* Generic collection functions */

static inline SLV2Collection
slv2_collection_new(GDestroyNotify destructor)
{
	return g_sequence_new(destructor);
}

SLV2_API
void
slv2_collection_free(SLV2Collection coll)
{
	if (coll)
		g_sequence_free((GSequence*)coll);
}

SLV2_API
unsigned
slv2_collection_size(SLV2Collection coll)
{
	return (coll ? g_sequence_get_length((GSequence*)coll) : 0);
}

SLV2_API
void*
slv2_collection_get_at(SLV2Collection coll, unsigned index)
{
	if (!coll || index >= slv2_collection_size(coll)) {
		return NULL;
	} else {
		GSequenceIter* i = g_sequence_get_iter_at_pos((GSequence*)coll, (int)index);
		return g_sequence_get(i);
	}
}

SLV2_API
SLV2Iter
slv2_collection_begin(SLV2Collection collection)
{
	return collection ? g_sequence_get_begin_iter(collection) : NULL;
}

SLV2_API
void*
slv2_collection_get(SLV2Collection collection,
                    SLV2Iter       i)
{
	return g_sequence_get((GSequenceIter*)i);
}

/* Constructors */

SLV2_API
SLV2ScalePoints
slv2_scale_points_new(void)
{
	return slv2_collection_new((GDestroyNotify)slv2_scale_point_free);
}

SLV2_API
SLV2Values
slv2_values_new(void)
{
	return slv2_collection_new((GDestroyNotify)slv2_value_free);
}

SLV2UIs
slv2_uis_new(void)
{
	return slv2_collection_new((GDestroyNotify)slv2_ui_free);
}

SLV2PluginClasses
slv2_plugin_classes_new(void)
{
	return slv2_collection_new((GDestroyNotify)slv2_plugin_class_free);
}

/* URI based accessors (for collections of things with URIs) */

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

/* Plugins */

SLV2Plugins
slv2_plugins_new()
{
	return g_sequence_new(NULL);
}

SLV2_API
SLV2Plugin
slv2_plugins_get_by_uri(SLV2Plugins list, SLV2Value uri)
{
	return (SLV2Plugin)slv2_sequence_get_by_uri(list, uri);
}

/* Values */

SLV2_API
bool
slv2_values_contains(SLV2Values list, SLV2Value value)
{
	SLV2_FOREACH(i, list)
		if (slv2_value_equals(slv2_values_get(list, i), value))
			return true;

	return false;
}

/* Iterator */

SLV2_API
SLV2Iter
slv2_iter_next(SLV2Iter i)
{
	return g_sequence_iter_next((GSequenceIter*)i);
}

SLV2_API
bool
slv2_iter_end(SLV2Iter i)
{
	return !i || g_sequence_iter_is_end((GSequenceIter*)i);
}

#define SLV2_COLLECTION_IMPL(prefix, CT, ET) \
SLV2_API \
unsigned \
prefix##_size(CT collection) { \
	return slv2_collection_size(collection); \
} \
\
SLV2_API \
SLV2Iter \
prefix##_begin(CT collection) { \
	return slv2_collection_begin(collection); \
} \
\
SLV2_API \
ET \
prefix##_get(CT collection, SLV2Iter i) { \
	return (ET)slv2_collection_get(collection, i); \
} \
\
SLV2_DEPRECATED \
SLV2_API \
ET \
prefix##_get_at(CT collection, unsigned index) { \
	return (ET)slv2_collection_get_at(collection, index); \
}

SLV2_COLLECTION_IMPL(slv2_plugin_classes, SLV2PluginClasses, SLV2PluginClass)
SLV2_COLLECTION_IMPL(slv2_scale_points, SLV2ScalePoints, SLV2ScalePoint)
SLV2_COLLECTION_IMPL(slv2_uis, SLV2UIs, SLV2UI)
SLV2_COLLECTION_IMPL(slv2_values, SLV2Values, SLV2Value)
SLV2_COLLECTION_IMPL(slv2_plugins, SLV2Plugins, SLV2Plugin)

SLV2_API
void
slv2_plugin_classes_free(SLV2PluginClasses collection) {
	slv2_collection_free(collection);
}

SLV2_API
void
slv2_scale_points_free(SLV2ScalePoints collection) {
	slv2_collection_free(collection);
}

SLV2_API
void
slv2_uis_free(SLV2UIs collection) {
	slv2_collection_free(collection);
}

SLV2_API
void
slv2_values_free(SLV2Values collection) {
	slv2_collection_free(collection);
}

SLV2_API
void
slv2_plugins_free(SLV2World world, SLV2Plugins plugins){
}

SLV2_API
SLV2Value
slv2_values_get_first(SLV2Values collection) {
	return (SLV2Value)slv2_collection_get(collection,
	                                      slv2_collection_begin(collection));
}
