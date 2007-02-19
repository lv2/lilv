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

#ifndef __SLV2_PLUGIN_H__
#define __SLV2_PLUGIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <slv2/types.h>
#include <slv2/stringlist.h>


typedef struct _Plugin* SLV2Plugin;


/** \defgroup data Plugin - RDF data access
 *
 * These functions work exclusively with the plugin's RDF data file.  They do
 * not load the plugin dynamic library (or access
 * it in any way).
 *
 * @{
 */


/** Check if this plugin is valid.
 *
 * This is used by plugin lists to avoid loading plugins that are not valid
 * and will not work with libslv2 (eg plugins missing required fields, or
 * having multiple values for mandatory single-valued fields, etc.
 * 
 * Note that normal hosts do NOT need to use this - slv2 does not
 * load invalid plugins into plugin lists.  This is included for plugin
 * testing utilities, etc.
 *
 * \return true if \a plugin is valid.
 */
bool
slv2_plugin_verify(SLV2Plugin plugin);


/** Duplicate a plugin.
 *
 * Use this if you want to keep an SLV2Plugin around but free the list it came
 * from.  Freeing the returned plugin with slv2_plugin_free is the caller's
 * responsibility.
 *
 * \return a newly allocated deep copy of \a plugin.
 */
SLV2Plugin
slv2_plugin_duplicate(SLV2Plugin plugin);


/** Free an SLV2Plugin.
 *
 * This should NEVER be used on a plugin contained in a Plugins.  Use this
 * only with plugins created with slv2_plugin_duplicate.
 */
void
slv2_plugin_free(SLV2Plugin plugin);


/** Get the URI of \a plugin.
 *
 * Any serialization that refers to plugins should refer to them by this.
 * Hosts SHOULD NOT save any filesystem paths, plugin indexes, etc. in saved
 * files; save only the URI.
 *
 * The URI is a globally unique identifier for one specific plugin.  Two
 * plugins with the same URI are compatible in port signature, and should
 * be guaranteed to work in a compatible and consistent way.  If a plugin
 * is upgraded in an incompatible way (eg if it has different ports), it
 * MUST have a different URI than it's predecessor.
 *
 * \return a shared string which must not be modified or free()'d.
 */
const char*
slv2_plugin_get_uri(SLV2Plugin plugin);


/** Get the (resolvable) URIs of the RDF data files that define a plugin.
 *
 * Note this always returns fully qualified URIs.  If you want local
 * filesystem paths, use slv2_uri_to_path.
 *
 * \return a complete URL eg. "file:///usr/foo/SomeBundle.lv2/someplug.ttl",
 * which is shared and must not be modified or free()'d.
 */
SLV2Strings
slv2_plugin_get_data_uris(SLV2Plugin plugin);


/** Get the (resolvable) URI of the shared library for \a plugin.
 *
 * Note this always returns a fully qualified URI.  If you want a local
 * filesystem path, use slv2_uri_to_path.
 *
 * \return a shared string which must not be modified or free()'d.
 */
const char*
slv2_plugin_get_library_uri(SLV2Plugin plugin);



/** Get the name of \a plugin.
 *
 * This is guaranteed to return the untranslated name (the doap:name in the
 * data file without a language tag).  Returned value must be free()'d by
 * the caller.
 */
char*
slv2_plugin_get_name(SLV2Plugin plugin);


/** Request some arbitrary RDF object of the plugin.
 *
 * May return NULL if the property was not found (ie is not defined in the
 * data file), or if object is not sensibly represented as an SLV2Strings
 * (e.g. blank nodes).
 *
 * Return value must be freed by caller with slv2_value_free.
 *
 * Note that this may return multiple values. If the property is a
 * string with multiple languages defined, the translation according to
 * $LANG will be returned if it is set.  Otherwise all values will be
 * returned.
 */
SLV2Strings
slv2_plugin_get_value(SLV2Plugin  p,
                      const char* predicate);


/** Get the LV2 Properties of a plugin.
 *
 * LV2 Properties are mandatory.  Hosts MUST NOT use a plugin if they do not
 * understand all the LV2 Properties associated with that plugin (if this is
 * not what you want, see slv2_plugin_get_hints).
 *
 * Return value must be freed by caller with slv2_value_free.
 */
SLV2Strings
slv2_plugin_get_properties(SLV2Plugin p);


/** Get the LV2 Hints of a plugin.
 *
 * LV2 Hints are suggestions that may be useful for a host.  LV2 Hints may be
 * ignored and the plugin will still function correctly.
 *
 * Return value must be freed by caller with slv2_value_free.
 */
SLV2Strings
slv2_plugin_get_hints(SLV2Plugin p);


/** Get the number of ports on this plugin.
 */
uint32_t
slv2_plugin_get_num_ports(SLV2Plugin p);


/** Return whether or not the plugin introduces (and reports) latency.
 *
 * The index of the latency port can be found with slv2_plugin_get_latency_port
 * ONLY if this function returns true.
 */
bool
slv2_plugin_has_latency(SLV2Plugin p);


/** Return the index of the plugin's latency port, or the empty string if the
 * plugin has no latency.
 *
 * It is a fatal error to call this on a plugin without checking if the port
 * exists by first calling slv2_plugin_has_latency.
 *
 * Any plugin that introduces unwanted latency that should be compensated for
 * (by hosts with the ability/need) MUST provide this port, which is a control
 * rate output port that reports the latency for each cycle in frames.
 */
uint32_t
slv2_plugin_get_latency_port(SLV2Plugin p);


/** Get a plugin's supported host features / extensions.
 *
 * This returns a list of all supported features (both required and optional).
 */
SLV2Strings
slv2_plugin_get_supported_features(SLV2Plugin p);


/** Get a plugin's requires host features / extensions.
 *
 * All feature URI's returned by this call MUST be passed to the plugin's
 * instantiate method for the plugin to instantiate successfully.
 */
SLV2Strings
slv2_plugin_get_required_features(SLV2Plugin p);


/** Get a plugin's optional host features / extensions.
 *
 * If the feature URI's returned by this method are passed to the plugin's
 * instantiate method, those features will be used by the function, otherwise
 * the plugin will act as it would if it did not support that feature at all.
 */
SLV2Strings
slv2_plugin_get_optional_features(SLV2Plugin p);


/** Query a plugin for a single variable.
 */
SLV2Strings
slv2_plugin_simple_query(SLV2Plugin  plugin,
                         const char* sparql_str,
                         const char* variable);


/** Query a plugin and return the number of results found.
 */
unsigned
slv2_plugin_query_count(SLV2Plugin  plugin,
                        const char* sparql_str);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PLUGIN_H__ */

