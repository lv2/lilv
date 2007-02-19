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

#ifdef __cplusplus
extern "C" {
#endif


typedef void* SLV2Plugins;


/** \defgroup plugins Plugins - Collection of plugins, plugin discovery
 * 
 * These functions are for locating plugins installed on the system.
 *
 * Normal hosts which just want to easily load plugins by URI are strongly
 * recommended to simply find all installed plugins with
 * \ref slv2_plugins_load_all rather than find and load bundles manually.
 * 
 * Functions are provided for hosts that wish to access bundles explicitly and
 * individually for some reason, as well as make custom lists of plugins from
 * a selection of bundles.  This is mostly intended for hosts which are
 * tied to a specific (bundled with the application) bundle.
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


/** Filter plugins from one list into another.
 *
 * All plugins in @a source that return true when passed to @a include
 * (a pointer to a function that takes an SLV2Plugin and returns a bool)
 * will be added to @a dest.  Plugins are duplicated into dest, it is safe
 * to destroy source and continue to use dest after this call.
 */
void
slv2_plugins_filter(SLV2Plugins dest,
                    SLV2Plugins source, 
                    bool (*include)(SLV2Plugin));


/** Add all plugins installed on the system to \a list.
 *
 * This is the recommended way for hosts to access plugins.  It does the most
 * reasonable thing to find all installed plugins on a system.  The environment
 * variable LV2_PATH may be set to control the locations this function will
 * look for plugins.
 *
 * Use of any functions for locating plugins other than this one is \em highly
 * discouraged without a special reason to do so (and is just more work for the
 * host author) - use this one.
 */
void
slv2_plugins_load_all(SLV2Plugins list);


/** Add all plugins found in \a search_path to \a list.
 *
 * If \a search_path is NULL, \a list will be unmodified.
 *
 * Use of this function is \b not recommended.  Use \ref slv2_plugins_load_all.
 */
void
slv2_plugins_load_path(SLV2Plugins list,
                       const char* search_path);


/** Add all plugins found in the bundle at \a bundle_base_url to \a list.
 *
 * \arg bundle_base_url is a fully qualified path to the bundle directory, eg.
 * "file:///usr/lib/lv2/someBundle"
 *
 * Use of this function is \b strongly discouraged - hosts should not attach
 * \em any significance to bundle paths as there are no guarantees they will
 * remain consistent whatsoever.  This function should only be used by apps
 * which ship with a special bundle (which it knows exists at some path).
 * It is \b not to be used by normal hosts that want to load system
 * installed plugins.  Use \ref slv2_plugins_load_all for that.
 */
void
slv2_plugins_load_bundle(SLV2Plugins list,
                         const char* bundle_base_uri);


/** Get the number of plugins in the list.
 */
unsigned
slv2_plugins_size(SLV2Plugins list);


/** Get a plugin from the list by URI.
 *
 * Return value is shared (stored in \a list) and must not be freed or
 * modified by the caller in any way.
 * This functions is a search, slv2_plugins_get_at is
 * significantly faster.
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

