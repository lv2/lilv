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

#ifndef __SLV2_WORLD_H__
#define __SLV2_WORLD_H__

#include <slv2/pluginlist.h>

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup world Library context, data loading, etc.
 * 
 * These functions deal with the data model which other SLV2 methods
 * operate with.  The world contains an in-memory cache of all bundles
 * manifest.ttl files, from which you can quickly query plugins, etc.
 *
 * Normal hosts which just want to easily load plugins by URI are strongly
 * recommended to simply find all installed data in the recommended way with
 * \ref slv2_world_load_all rather than find and load bundles manually.
 * 
 * Functions are provided for hosts that wish to access bundles explicitly and
 * individually for some reason, this is intended for hosts which are tied to
 * a specific bundle (shipped with the application).
 *
 * @{
 */


/** Initialize a new, empty world.
 */
SLV2World
slv2_world_new();


/** Destroy the world, mwahaha.
 *
 * NB: Destroying the world will leave dangling references in any plugin lists,
 * plugins, etc.  Do not destroy the world until you are finished with all
 * objects that came from it.
 */
void
slv2_world_free(SLV2World world);


/** Load all installed LV2 bundles on the system
 *
 * This is the recommended way for hosts to load LV2 data.  It does the most
 * reasonable thing to find all installed plugins, extensions, etc. on the
 * system.  The environment variable LV2_PATH may be used to set the
 * directories inside which this function will look for bundles.  Otherwise
 * a sensible, standard default will be used.
 *
 * Use of other functions for loading bundles is \em highly discouraged
 * without a special reason to do so - use this one.
 */
void
slv2_world_load_all(SLV2World world);


/** Load all bundles found in \a search_path.
 *
 * \param search_path A colon-delimited list of directories.  These directories
 * should contain LV2 bundle directories (ie the search path is a list of
 * parent directories of bundles, not a list of bundle directories).
 *
 * If \a search_path is NULL, \a world will be unmodified.
 * Use of this function is \b not recommended.  Use \ref slv2_world_load_all.
 */
void
slv2_world_load_path(SLV2World   world,
                     const char* search_path);


/** Load a specific bundle into \a world.
 *
 * \arg bundle_base_uri is a fully qualified URI to the bundle directory,
 * with the trailing slash, eg. file:///usr/lib/lv2/someBundle/
 *
 * Normal hosts should not use this function.
 *
 * Hosts should not attach \em any long-term significance to bundle paths
 * as there are no guarantees they will remain consistent whatsoever.
 * This function should only be used by apps which ship with a special
 * bundle (which it knows exists at some path because they are part of
 * the same package).
 */
void
slv2_world_load_bundle(SLV2World   world,
                       const char* bundle_base_uri);


/** Add all plugins present in \a world to \a list.
 *
 * Returned plugins contain a reference to this world, world must not be
 * destroyed until plugins are finished with.
 */
SLV2Plugins
slv2_world_get_all_plugins(SLV2World world);


/** Get plugins filtered by a user-defined filter function.
 *
 * All plugins in \a world that return true when passed to \a include
 * (a pointer to a function that takes an SLV2Plugin and returns a bool)
 * will be added to \a list.
 *
 * Returned plugins contain a reference to this world, world must not be
 * destroyed until plugins are finished with.
 */
SLV2Plugins
slv2_world_get_plugins_by_filter(SLV2World world,
                                 bool (*include)(SLV2Plugin));


#if 0
/** Get plugins filtered by a user-defined SPARQL query.
 *
 * This is much faster than using slv2_world_get_plugins_by_filter with a
 * filter function which calls the various slv2_plugin_* functions.
 *
 * \param query A valid SPARQL query which SELECTs a single variable, which
 * should match the URI of plugins to be loaded.
 *
 * \b Example: Get all plugins with at least 1 audio input and output:
<tt> \verbatim
PREFIX : <http://lv2plug.in/ontology#>
SELECT DISTINCT ?plugin WHERE {
    ?plugin  :port  [ a :AudioPort; a :InputPort ] ;
             :port  [ a :AudioPort; a :OutputPort ] .
}
\endverbatim </tt>
 *
 * Returned plugins contain a reference to this world, world must not be
 * destroyed until plugins are finished with.
 */
SLV2Plugins
slv2_world_get_plugins_by_query(SLV2World   world,
                                const char* query);
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_WORLD_H__ */

