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

#ifndef __SLV2_MODEL_H__
#define __SLV2_MODEL_H__

#include <slv2/pluginlist.h>

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup model Data model loading
 * 
 * These functions deal with the data model which other SLV2 methods
 * operate with.  The data model is LV2 data loaded from bundles, from
 * which you can query plugins, etc.
 *
 * Normal hosts which just want to easily load plugins by URI are strongly
 * recommended to simply find all installed data in the recommended way with
 * \ref slv2_model_load_all rather than find and load bundles manually.
 * 
 * Functions are provided for hosts that wish to access bundles explicitly and
 * individually for some reason, this is intended for hosts which are tied to
 * a specific bundle (shipped with the application).
 *
 * @{
 */


/** Create a new, empty model.
 */
SLV2Model
slv2_model_new();


/** Destroy a model.
 *
 * NB: Destroying a model will leave dangling references in any plugin lists,
 * plugins, etc.  Do not destroy a model until you are finished with all
 * objects that came from it.
 */
void
slv2_model_free(SLV2Model model);


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
slv2_model_load_all(SLV2Model model);


/** Load all bundles found in \a search_path.
 *
 * \param search_path A colon-delimited list of directories.  These directories
 * should contain LV2 bundle directories (ie the search path is a list of
 * parent directories of bundles, not a list of bundle directories).
 *
 * If \a search_path is NULL, \a model will be unmodified.
 * Use of this function is \b not recommended.  Use \ref slv2_model_load_all.
 */
void
slv2_model_load_path(SLV2Model   model,
                     const char* search_path);


/** Load a specific bundle into \a model.
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
slv2_model_load_bundle(SLV2Model   model,
                       const char* bundle_base_uri);


/** Add all plugins present in \a model to \a list.
 *
 * Returned plugins contain a reference to this model, model must not be
 * destroyed until plugins are finished with.
 */
SLV2Plugins
slv2_model_get_all_plugins(SLV2Model model);


/** Get plugins filtered by a user-defined filter function.
 *
 * All plugins in \a model that return true when passed to \a include
 * (a pointer to a function that takes an SLV2Plugin and returns a bool)
 * will be added to \a list.
 *
 * Returned plugins contain a reference to this model, model must not be
 * destroyed until plugins are finished with.
 */
SLV2Plugins
slv2_model_get_plugins_by_filter(SLV2Model   model,
                                 bool (*include)(SLV2Plugin));


#if 0
/** Get plugins filtered by a user-defined SPARQL query.
 *
 * This is much faster than using slv2_model_get_plugins_by_filter with a
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
 * Returned plugins contain a reference to this model, model must not be
 * destroyed until plugins are finished with.
 */
SLV2Plugins
slv2_model_get_plugins_by_query(SLV2Model   model,
                                const char* query);
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_MODEL_H__ */

