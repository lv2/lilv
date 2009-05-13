/* SLV2
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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

#ifndef __SLV2_PLUGINS_H__
#define __SLV2_PLUGINS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "slv2/types.h"
#include "slv2/value.h"

/** \defgroup slv2_collections Collections of values/objects
 *
 * Ordered collections of typed values which are fast for random
 * access by index (i.e. a fancy array).
 *
 * @{
 */


/* **** GENERIC COLLECTION FUNCTIONS **** */

#define SLV2_COLLECTION(CollType, ElemType, prefix) \
\
/** Free a collection.
 *
 * Time = O(1)
 */ \
void \
prefix ## _free(CollType collection); \
\
\
/** Get the number of elements in the collection.
 *
 * Time = O(1)
 */ \
unsigned \
prefix ## _size(CollType collection); \
\
\
/** Get an element from the collection by index.
 *
 * \a index has no significance other than as an index into this collection.
 * Any \a index not less than the size of the collection will return NULL,
 * so all elements in a collection can be enumerated by repeated calls
 * to this function starting with \a index = 0.
 *
 * Time = O(1)
 *
 * \return NULL if \a index out of range.
 */ \
ElemType \
prefix ## _get_at(CollType collection, \
                  unsigned    index);

SLV2_COLLECTION(SLV2PluginClasses, SLV2PluginClass, slv2_plugin_classes)
SLV2_COLLECTION(SLV2ScalePoints, SLV2ScalePoint, slv2_scale_points)
SLV2_COLLECTION(SLV2Values, SLV2Value, slv2_values)
SLV2_COLLECTION(SLV2UIs, SLV2UI, slv2_uis)



/* **** PLUGINS **** */

/** Free a plugin collection.
 *
 * Freeing a plugin collection does not destroy the plugins it contains
 * (plugins are owned by the world). \a plugins is invalid after this call.
 * Time = O(1)
 */
void
slv2_plugins_free(SLV2World   world,
                  SLV2Plugins plugins);


/** Get the number of plugins in the collection.
 * Time = O(1)
 */
unsigned
slv2_plugins_size(SLV2Plugins plugins);


/** Get a plugin from the collection by URI.
 *
 * Return value is shared (stored in \a plugins) and must not be freed or
 * modified by the caller in any way.
 *
 * Time = O(log2(n))
 *
 * \return NULL if plugin with \a url not found in \a plugins.
 */
SLV2Plugin
slv2_plugins_get_by_uri(SLV2Plugins plugins,
                        SLV2Value   uri);


/** Get a plugin from the plugins by index.
 *
 * \a index has no significance other than as an index into this plugins.
 * Any \a index not less than slv2_plugins_get_length(plugins) will return NULL,
 * so all plugins in a plugins can be enumerated by repeated calls
 * to this function starting with \a index = 0.
 *
 * Time = O(1)
 *
 * \return NULL if \a index out of range.
 */
SLV2Plugin
slv2_plugins_get_at(SLV2Plugins plugins,
                    unsigned    index);



/* **** PLUGIN CLASSES **** */

/** Get a plugin class from the collection by URI.
 *
 * Return value is shared (stored in \a classes) and must not be freed or
 * modified by the caller in any way.
 *
 * Time = O(log2(n))
 *
 * \return NULL if plugin with \a url not found in \a classes.
 */
SLV2PluginClass
slv2_plugin_classes_get_by_uri(SLV2PluginClasses classes,
                               SLV2Value         uri);



/* **** SCALE POINTS **** */

/** Allocate a new, empty SLV2ScalePoints
 */
SLV2ScalePoints
slv2_scale_points_new();



/* **** VALUES **** */

/** Allocate a new, empty SLV2Values
 */
SLV2Values
slv2_values_new();


/** Return whether \a values contains \a value.
 *
 * Time = O(n)
 */
bool
slv2_values_contains(SLV2Values values, SLV2Value value);



/* **** PLUGIN UIS **** */

/** Get a plugin from the list by URI.
 *
 * Return value is shared (stored in \a list) and must not be freed or
 * modified by the caller in any way.
 *
 * Time = O(log2(n))
 *
 * \return NULL if plugin with \a url not found in \a list.
 */
SLV2UI
slv2_uis_get_by_uri(SLV2UIs   list,
                    SLV2Value uri);


/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SLV2_COLLECTIONS_H__ */
