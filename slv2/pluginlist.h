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


typedef struct _PluginList* SLV2List;


/** \defgroup lists Plugin discovery
 *
 * These functions are for locating plugins installed on the system.
 *
 * Normal hosts which just want to easily load plugins by URI are strongly
 * recommended to simply find all installed plugins with
 * \ref slv2_list_load_all rather than find and load bundles manually.
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
 * Returned object must be freed with slv2_list_free.
 */
SLV2List
slv2_list_new();


/** Free a plugin list.
 *
 * Note that all plugins in the list (eg those returned by the get_plugin
 * functions) will be deleted as well.  It is expected that hosts will
 * keep the plugin list allocated until they are done with their plugins.
 * If you want to keep a plugin around, but free the list it came from, you
 * will have to copy it with slv2_plugin_duplicate().
 *
 * \a list is invalid after this call (though it may be used again after a
 * "list = slv2_list_new()")
 */
void
slv2_list_free(SLV2List list);


/** Add all plugins installed on the system to \a list.
 *
 * This is the recommended way for hosts to access plugins.  It finds all
 * plugins on the system using the recommended mechanism.  At the time, this
 * is by searching the path defined in the environment variable LADSPA2_PATH,
 * though this is subject to change in the future.  Future versions may, for
 * example, allow users to specify a plugin whitelist of plugins they would
 * like to be visible in apps (or conversely a blacklist of plugins they do
 * not wish to use).
 *
 * Use of any functions for locating plugins other than this one is \em highly
 * discouraged without a special reason to do so - use this one.
 */
void
slv2_list_load_all(SLV2List list);


/** Add all plugins found in \a search_path to \a list.
 *
 * If \a search_path is NULL, \a list will be unmodified.
 *
 * Use of this function is \b not recommended.  Use \ref slv2_list_load_all.
 */
void
slv2_list_load_path(SLV2List    list,
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
 * installed plugins.  Use \ref slv2_list_load_all for that.
 */
void
slv2_list_load_bundle(SLV2List    list,
                      const char* bundle_base_url);


/** Get the number of plugins in the list.
 */
size_t
slv2_list_get_length(const SLV2List list);


/** Get a plugin from the list by URI.
 *
 * Return value is shared (stored in \a list) and must not be freed or
 * modified by the caller in any way.
 * 
 * \return NULL if plugin with \a url not found in \a list.
 */
const SLV2Plugin*
slv2_list_get_plugin_by_uri(const SLV2List list,
                            const char*    uri);


/** Get a plugin from the list by index.
 *
 * \a index has no significance.  Any \a index not less than
 * slv2list_get_length(list) will return NULL.  * All plugins in a list can
 * thus be easily enumerated by repeated calls to this function starting
 * with \a index 0.
 *
 * Return value is shared (stored in \a list) and must not be freed or
 * modified by the caller in any way.
 *
 * \return NULL if \a index out of range.
 */
const SLV2Plugin*
slv2_list_get_plugin_by_index(const SLV2List list,
                              size_t         index);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PLUGINLIST_H__ */

