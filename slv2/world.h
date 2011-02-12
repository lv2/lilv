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

/** @defgroup slv2_world Global library state
 *
 * The "world" represents all library state, and the data found in bundles'
 * manifest.ttl (ie it is an in-memory index of all things LV2 found).
 * Plugins (and plugin extensions) and the LV2 specification (and LV2
 * extensions) itself can be queried from the world for use.
 *
 * Normal hosts which just want to easily load plugins by URI are strongly
 * recommended to simply call @ref slv2_world_load_all to find all installed
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
 * Note that destroying @a world will destroy all the objects it contains
 * (e.g. instances of SLV2Plugin).  Do not destroy the world until you are
 * finished with all objects that came from it.
 */
SLV2_API
void
slv2_world_free(SLV2World world);

/** Load all installed LV2 bundles on the system.
 * This is the recommended way for hosts to load LV2 data.  It implements the
 * established/standard best practice for discovering all LV2 data on the
 * system.  The environment variable LV2_PATH may be used to control where
 * this function will look for bundles.
 *
 * Hosts should use this function rather than explicitly load bundles, except
 * in special circumstances (e.g. development utilities, or hosts that ship
 * with special plugin bundles which are installed to a known location).
 */
SLV2_API
void
slv2_world_load_all(SLV2World world);

/** Load a specific bundle.
 * @a bundle_uri must be a fully qualified URI to the bundle directory,
 * with the trailing slash, eg. file:///usr/lib/lv2/foo.lv2/
 *
 * Normal hosts should not need this function (use slv2_world_load_all).
 *
 * Hosts MUST NOT attach any long-term significance to bundle paths
 * (e.g. in save files), since there are no guarantees they will remain
 * unchanged between (or even during) program invocations. Plugins (among
 * other things) MUST be identified by URIs (not paths) in save files.
 */
SLV2_API
void
slv2_world_load_bundle(SLV2World world,
                       SLV2Value bundle_uri);

/** Get the parent of all other plugin classes, lv2:Plugin.
 */
SLV2_API
SLV2PluginClass
slv2_world_get_plugin_class(SLV2World world);

/** Return a list of all found plugin classes.
 * Returned list is owned by world and must not be freed by the caller.
 */
SLV2_API
SLV2PluginClasses
slv2_world_get_plugin_classes(SLV2World world);

/** Return a list of all found plugins.
 * The returned list contains just enough references to query
 * or instantiate plugins.  The data for a particular plugin will not be
 * loaded into memory until a call to an slv2_plugin_* function results in
 * a query (at which time the data is cached with the SLV2Plugin so future
 * queries are very fast).
 *
 * Returned list must be freed by user with slv2_plugins_free.  The contained
 * plugins are owned by @a world and must not be freed by caller.
 */
SLV2_API
SLV2Plugins
slv2_world_get_all_plugins(SLV2World world);

/** Return a list of found plugins filtered by a user-defined filter function.
 * All plugins currently found in @a world that return true when passed to
 * @a include (a pointer to a function which takes an SLV2Plugin and returns
 * a bool) will be in the returned list.
 *
 * Returned list must be freed by user with slv2_plugins_free.  The contained
 * plugins are owned by @a world and must not be freed by caller.
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
