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

#ifndef __SLV2_PLUGINLIST_H__
#define __SLV2_PLUGINLIST_H__

#include <slv2/plugin.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void* SLV2Plugins;


/** \defgroup plugins Plugin lists
 * 
 * These functions work with lists of plugins which come from an
 * SLV2Model.  These lists contain only a weak reference to an LV2 plugin
 * in the Model.
 *
 * @{
 */


/** Create a new, empty plugin list.
 *
 * Returned object must be freed with slv2_plugins_free.
 */
SLV2Plugins
slv2_plugins_new();


/** Free a plugin list.
 *
 * Note that all plugins in the list (eg those returned by the get_plugin
 * functions) will be deleted as well.  It is expected that hosts will
 * keep the plugin list allocated until they are done with their plugins.
 * If you want to keep a plugin around, but free the list it came from, you
 * will have to copy it with slv2_plugin_duplicate().
 *
 * \a list is invalid after this call (though it may be used again after a
 * "list = slv2_plugins_new()")
 */
void
slv2_plugins_free(SLV2Plugins list);


/** Get the number of plugins in the list.
 */
unsigned
slv2_plugins_size(SLV2Plugins list);


/** Get a plugin from the list by URI.
 *
 * Return value is shared (stored in \a list) and must not be freed or
 * modified by the caller in any way.
 *
 * O(log2(n))
 * 
 * \return NULL if plugin with \a url not found in \a list.
 */
SLV2Plugin
slv2_plugins_get_by_uri(SLV2Plugins list,
                        const char* uri);


/** Get a plugin from the list by index.
 *
 * \a index has no significance.  Any \a index not less than
 * slv2list_get_length(list) will return NULL.  All plugins in a list can
 * thus be easily enumerated by repeated calls to this function starting
 * with \a index 0.
 *
 * Return value is shared (stored in \a list) and must not be freed or
 * modified by the caller in any way.
 *
 * O(1)
 *
 * \return NULL if \a index out of range.
 */
SLV2Plugin
slv2_plugins_get_at(SLV2Plugins list,
                    unsigned    index);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PLUGINLIST_H__ */

