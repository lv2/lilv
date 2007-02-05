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
#include <stddef.h>
#include <stdbool.h>
#include "types.h"


typedef const struct _Plugin SLV2Plugin;


/** \defgroup data Plugin data file access
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
 * Note that normal hosts do not need to worry about this - libslv2 does not
 * load invalid plugins into plugin lists.  This is included for plugin
 * testing utilities, etc.
 *
 * \return True if \a plugin is valid.
 */
bool
slv2_plugin_verify(const SLV2Plugin* plugin);


/** Duplicate a plugin.
 *
 * Use this if you want to keep an SLV2Plugin around but free the list it came
 * from.
 *
 * \return a newly allocated deep copy of \a plugin.
 */
SLV2Plugin*
slv2_plugin_duplicate(const SLV2Plugin* plugin);


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
slv2_plugin_get_uri(const SLV2Plugin* plugin);


/** Get the URL of the RDF data file plugin information is located in.
 *
 * Only file: URL's are supported at this time.
 *
 * \return a complete URL eg. "file:///usr/foo/SomeBundle.lv2/someplug.ttl",
 * which is shared and must not be modified or free()'d.
 */
const char*
slv2_plugin_get_data_url(const SLV2Plugin* plugin);


/** Get the local filesystem path of the RDF data file for \a plugin.
 *
 * \return a valid path on the local filesystem
 * eg. "/usr/foo/SomeBundle.lv2/someplug.ttl" which is shared and must not
 * be free()'d; or NULL if URL is not a local filesystem path.
 */
const char*
slv2_plugin_get_data_path(const SLV2Plugin* plugin);


/** Get the URL of the shared library for \a plugin.
 *
 * \return a shared string which must not be modified or free()'d.
 */
const char*
slv2_plugin_get_library_url(const SLV2Plugin* plugin);


/** Get the local filesystem path of the shared library for \a plugin.
 *
 * \return a valid path on the local filesystem
 * eg. "/usr/foo/SomeBundle.lv2/someplug.so" which is shared and must not
 * be free()'d; or NULL if URL is not a local filesystem path.
 */
const char*
slv2_plugin_get_library_path(const SLV2Plugin* plugin);


/** Get the name of \a plugin.
 *
 * This is guaranteed to return the untranslated name (the doap:name in the
 * data file without a language tag).  Returned value must be free()'d by
 * the caller.
 */
char*
slv2_plugin_get_name(const SLV2Plugin* plugin);


/** Request some property of the plugin.
 *
 * May return NULL if the property was not found (ie is not defined in the
 * data file).
 *
 * Return value must be free()'d by caller.
 *
 * Note that some properties may have multiple values. If the property is a
 * string with multiple languages defined, the translation according to
 * $LANG will be returned if it is set.  Otherwise all values will be
 * returned.
 */
SLV2Property
slv2_plugin_get_property(const SLV2Plugin* p,
                         const char*       property);


/** Get the number of ports on this plugin.
 */
uint32_t
slv2_plugin_get_num_ports(const SLV2Plugin* p);

/** Return whether or not the plugin introduces (and reports) latency.
 *
 * The index of the latency port can be found with slv2_plugin_get_latency_port
 * ONLY if this function returns true.
 */
bool
slv2_plugin_has_latency(const SLV2Plugin* p);

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
slv2_plugin_get_latency_port(const SLV2Plugin* p);

#if 0
/** Return whether or not a plugin supports the given host feature / extension.
 *
 * This will return true for both supported and required host features.
 */
bool
slv2_plugin_supports_feature(const SLV2Plugin* p, const char* feature_uri);


/** Return whether or not a plugin requires the given host feature / extension.
 *
 * If a plugin requires a feature, that feature MUST be passed to the plugin's
 * instantiate method or the plugin will fail to instantiate.
 */
bool
slv2_plugin_requires_features(const SLV2Plugin* p, const char* feature_uri);
#endif

/** Get a plugin's supported host features / extensions.
 *
 * This returns a list of all supported features (both required and optional).
 */
SLV2Property
slv2_plugin_get_supported_features(const SLV2Plugin* p);


/** Get a plugin's requires host features / extensions.
 *
 * All feature URI's returned by this call MUST be passed to the plugin's
 * instantiate method for the plugin to instantiate successfully.
 */
SLV2Property
slv2_plugin_get_required_features(const SLV2Plugin* p);


/** Get a plugin's optional host features / extensions.
 *
 * If the feature URI's returned by this method are passed to the plugin's
 * instantiate method, those features will be used by the function, otherwise
 * the plugin will act as it would if it did not support that feature at all.
 */
SLV2Property
slv2_plugin_get_optional_features(const SLV2Plugin* p);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PLUGIN_H__ */

