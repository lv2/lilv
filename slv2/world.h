/* SLV2
 * Copyright (C) 2007-2011 David Robillard <http://drobilla.net>
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

#ifdef __cplusplus
extern "C" {
#endif

#include "slv2/collections.h"

/** \defgroup slv2_world Global library state
 *
 * The "world" represents all library state, and the data found in bundles'
 * manifest.ttl (ie it is an in-memory index of all things LV2 found).
 * Plugins (and plugin extensions) and the LV2 specification (and LV2
 * extensions) itself can be queried from the world for use.
 *
 * Normal hosts which just want to easily load plugins by URI are strongly
 * recommended to simply call \ref slv2_world_load_all to find all installed
 * data in the recommended way.
 *
 * Normal hosts should NOT have to refer to bundles directly under normal
 * circumstances.  However, functions are provided to load individual bundles
 * explicitly, intended for hosts which depend on a specific bundle
 * (which is shipped with the application).
 *
 * @{
 */

/** Initialize a new, empty world.
 *
 * If initialization fails, NULL is returned.
 */
SLV2_API
SLV2World
slv2_world_new(void);

/** Enable/disable language filtering.
 * Language filtering applies to any functions that return (a) value(s).
 * With filtering enabled, SLV2 will automatically return the best value(s)
 * for the current LANG.  With filtering disabled, all matching values will
 * be returned regardless of language tag.  Filtering is enabled by default.
 */
#define SLV2_OPTION_FILTER_LANG "http://drobilla.net/ns/slv2#filter-lang"

/** Set an SLV2 option for @a world.
 */
SLV2_API
void
slv2_world_set_option(SLV2World       world,
                      const char*     uri,
                      const SLV2Value value);

/** Destroy the world, mwahaha.
 *
 * NB: Destroying the world will leave dangling references in any plugin lists,
 * plugins, etc.  Do not destroy the world until you are finished with all
 * objects that came from it.
 */
SLV2_API
void
slv2_world_free(SLV2World world);

/** Load all installed LV2 bundles on the system.
 *
 * This is the recommended way for hosts to load LV2 data.  It does the most
 * reasonable thing to find all installed plugins, extensions, etc. on the
 * system.  The environment variable LV2_PATH may be used to set the
 * directories inside which this function will look for bundles.  Otherwise
 * a sensible, standard default will be used.
 *
 * Use of other functions for loading bundles is \em highly discouraged
 * without a special reason to do so - use this one.
 *
 * Time = Query
 */
SLV2_API
void
slv2_world_load_all(SLV2World world);

/** Load a specific bundle.
 *
 * \arg bundle_uri A fully qualified URI to the bundle directory,
 * with the trailing slash, eg. file:///usr/lib/lv2/someBundle/
 *
 * Normal hosts should not use this function.
 *
 * Hosts should \b never attach any long-term significance to bundle paths
 * as there are no guarantees they will remain consistent whatsoever.
 * Plugins (and other things) are identified by URIs, \b not bundle or
 * file names.
 *
 * This function should only be used by apps which ship with a special
 * bundle (which it knows exists at some path because the bundle is
 * shipped with the application).
 *
 * Time = Query
 */
SLV2_API
void
slv2_world_load_bundle(SLV2World world,
                       SLV2Value bundle_uri);

/** Get the parent of all other plugin classes, lv2:Plugin.
 *
 * Time = O(1)
 */
SLV2_API
SLV2PluginClass
slv2_world_get_plugin_class(SLV2World world);

/** Return a list of all found plugin classes.
 *
 * Returned list is owned by world and must not be freed by the caller.
 *
 * Time = O(1)
 */
SLV2_API
SLV2PluginClasses
slv2_world_get_plugin_classes(SLV2World world);

/** Return a list of all found plugins.
 *
 * The returned list contains just enough references to query
 * or instantiate plugins.  The data for a particular plugin will not be
 * loaded into memory until a call to an slv2_plugin_* function results in
 * a query (at which time the data is cached with the SLV2Plugin so future
 * queries are very fast).
 *
 * Returned list must be freed by user with slv2_plugins_free.  The contained
 * plugins are owned by \a world and must not be freed by caller.
 *
 * Time = O(1)
 */
SLV2_API
SLV2Plugins
slv2_world_get_all_plugins(SLV2World world);

/** Return a list of found plugins filtered by a user-defined filter function.
 *
 * All plugins currently found in \a world that return true when passed to
 * \a include (a pointer to a function which takes an SLV2Plugin and returns
 * a bool) will be in the returned list.
 *
 * Returned list must be freed by user with slv2_plugins_free.  The contained
 * plugins are owned by \a world and must not be freed by caller.
 *
 * Time = O(n * Time(include))
 */
SLV2_API
SLV2Plugins
slv2_world_get_plugins_by_filter(SLV2World world,
                                 bool (*include)(SLV2Plugin));

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SLV2_WORLD_H__ */
