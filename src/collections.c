/*
  Copyright 2008-2011 David Robillard <http://drobilla.net>

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

#include <glib.h>

#include "lilv_internal.h"

/* Generic collection functions */

static inline LilvCollection*
lilv_collection_new(GDestroyNotify destructor)
{
	return g_sequence_new(destructor);
}

void
lilv_collection_free(LilvCollection* coll)
{
	if (coll)
		g_sequence_free((GSequence*)coll);
}

unsigned
lilv_collection_size(const LilvCollection* coll)
{
	return (coll ? g_sequence_get_length((GSequence*)coll) : 0);
}

LilvIter*
lilv_collection_begin(const LilvCollection* collection)
{
	return collection ? g_sequence_get_begin_iter((LilvCollection*)collection) : NULL;
}

void*
lilv_collection_get(const LilvCollection* collection,
                    const LilvIter*       i)
{
	return g_sequence_get((GSequenceIter*)i);
}

/* Constructors */

LilvScalePoints*
lilv_scale_points_new(void)
{
	return lilv_collection_new((GDestroyNotify)lilv_scale_point_free);
}

LilvNodes*
lilv_nodes_new(void)
{
	return lilv_collection_new((GDestroyNotify)lilv_node_free);
}

LilvUIs*
lilv_uis_new(void)
{
	return lilv_collection_new((GDestroyNotify)lilv_ui_free);
}

LilvPluginClasses*
lilv_plugin_classes_new(void)
{
	return lilv_collection_new((GDestroyNotify)lilv_plugin_class_free);
}

/* URI based accessors (for collections of things with URIs) */

LILV_API
const LilvPluginClass*
lilv_plugin_classes_get_by_uri(const LilvPluginClasses* coll, const LilvNode* uri)
{
	return (LilvPluginClass*)lilv_sequence_get_by_uri(coll, uri);
}

LILV_API
const LilvUI*
lilv_uis_get_by_uri(const LilvUIs* coll, const LilvNode* uri)
{
	return (LilvUI*)lilv_sequence_get_by_uri((LilvUIs*)coll, uri);
}

/* Plugins */

LilvPlugins*
lilv_plugins_new(void)
{
	return g_sequence_new(NULL);
}

LILV_API
const LilvPlugin*
lilv_plugins_get_by_uri(const LilvPlugins* list, const LilvNode* uri)
{
	return (LilvPlugin*)lilv_sequence_get_by_uri((LilvPlugins*)list, uri);
}

/* Values */

LILV_API
bool
lilv_nodes_contains(const LilvNodes* list, const LilvNode* value)
{
	LILV_FOREACH(nodes, i, list)
		if (lilv_node_equals(lilv_nodes_get(list, i), value))
			return true;

	return false;
}

/* Iterator */

LilvIter*
lilv_iter_next(LilvIter* i)
{
	return g_sequence_iter_next((GSequenceIter*)i);
}

bool
lilv_iter_end(LilvIter* i)
{
	return !i || g_sequence_iter_is_end((GSequenceIter*)i);
}

#define LILV_COLLECTION_IMPL(prefix, CT, ET) \
LILV_API \
unsigned \
prefix##_size(const CT* collection) { \
	return lilv_collection_size(collection); \
} \
\
LILV_API \
LilvIter* \
prefix##_begin(const CT* collection) { \
	return lilv_collection_begin(collection); \
} \
\
LILV_API \
const ET* \
prefix##_get(const CT* collection, LilvIter* i) { \
	return (ET*)lilv_collection_get(collection, i); \
} \
\
LILV_API \
LilvIter* \
prefix##_next(const CT* collection, LilvIter* i) { \
	return lilv_iter_next(i); \
} \
\
LILV_API \
bool \
prefix##_is_end(const CT* collection, LilvIter* i) { \
	return lilv_iter_end(i); \
}

LILV_COLLECTION_IMPL(lilv_plugin_classes, LilvPluginClasses, LilvPluginClass)
LILV_COLLECTION_IMPL(lilv_scale_points, LilvScalePoints, LilvScalePoint)
LILV_COLLECTION_IMPL(lilv_uis, LilvUIs, LilvUI)
LILV_COLLECTION_IMPL(lilv_nodes, LilvNodes, LilvNode)
LILV_COLLECTION_IMPL(lilv_plugins, LilvPlugins, LilvPlugin)

LILV_API
void
lilv_plugin_classes_free(LilvPluginClasses* collection) {
	lilv_collection_free(collection);
}

LILV_API
void
lilv_scale_points_free(LilvScalePoints* collection) {
	lilv_collection_free(collection);
}

LILV_API
void
lilv_uis_free(LilvUIs* collection) {
	lilv_collection_free(collection);
}

LILV_API
void
lilv_nodes_free(LilvNodes* collection) {
	lilv_collection_free(collection);
}

LILV_API
LilvNode*
lilv_nodes_get_first(const LilvNodes* collection) {
	return (LilvNode*)lilv_collection_get(collection,
	                                      lilv_collection_begin(collection));
}
