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

#ifndef SLV2_SLV2_H__
#define SLV2_SLV2_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#if defined _WIN32 || defined __CYGWIN__
	#define SLV2_LIB_IMPORT __declspec(dllimport)
	#define SLV2_LIB_EXPORT __declspec(dllexport)
#else
	#define SLV2_LIB_IMPORT __attribute__ ((visibility("default")))
	#define SLV2_LIB_EXPORT __attribute__ ((visibility("default")))
#endif

#ifdef SLV2_SHARED  // Building a shared library
	#ifdef SLV2_INTERNAL  // Building SLV2 (not using it)
		#define SLV2_API SLV2_LIB_EXPORT
	#else
		#define SLV2_API SLV2_LIB_IMPORT
	#endif
#else  // Building a static library
	#define SLV2_API
#endif

#define SLV2_NAMESPACE_LV2      "http://lv2plug.in/ns/lv2core#"
#define SLV2_PORT_CLASS_PORT    "http://lv2plug.in/ns/lv2core#Port"
#define SLV2_PORT_CLASS_INPUT   "http://lv2plug.in/ns/lv2core#InputPort"
#define SLV2_PORT_CLASS_OUTPUT  "http://lv2plug.in/ns/lv2core#OutputPort"
#define SLV2_PORT_CLASS_CONTROL "http://lv2plug.in/ns/lv2core#ControlPort"
#define SLV2_PORT_CLASS_AUDIO   "http://lv2plug.in/ns/lv2core#AudioPort"
#define SLV2_PORT_CLASS_EVENT   "http://lv2plug.in/ns/ext/event#EventPort"
#define SLV2_EVENT_CLASS_MIDI   "http://lv2plug.in/ns/ext/midi#MidiEvent"

typedef struct _SLV2Plugin*       SLV2Plugin;       /**< LV2 Plugin. */
typedef struct _SLV2PluginClass*  SLV2PluginClass;  /**< Plugin Class. */
typedef struct _SLV2Port*         SLV2Port;         /**< Port. */
typedef struct _SLV2ScalePoint*   SLV2ScalePoint;   /**< Scale Point (Notch). */
typedef struct _SLV2Value*        SLV2Value;        /**< Typed Value. */
typedef struct _SLV2World*        SLV2World;        /**< SLV2 World. */

typedef void* SLV2PluginClasses;  /**< set<PluginClass>. */
typedef void* SLV2Plugins;        /**< set<Plugin>. */
typedef void* SLV2ScalePoints;    /**< array<ScalePoint>. */
typedef void* SLV2Values;         /**< array<Value>. */

/** @defgroup slv2 SLV2
 * SLV2 is a simple yet powerful C API for using LV2 plugins.
 *
 * For more information about LV2, see <http://lv2plug.in>.
 * For more information about SLV2, see <http://drobilla.net/software/slv2>.
 * @{
 */

/** @name Value
 * @{
 */

/** Create a new URI value.
 * Returned value must be freed by caller with slv2_value_free.
 */
SLV2_API
SLV2Value
slv2_value_new_uri(SLV2World world, const char* uri);

/** Create a new string value (with no language).
 * Returned value must be freed by caller with slv2_value_free.
 */
SLV2_API
SLV2Value
slv2_value_new_string(SLV2World world, const char* str);

/** Create a new integer value.
 * Returned value must be freed by caller with slv2_value_free.
 */
SLV2_API
SLV2Value
slv2_value_new_int(SLV2World world, int val);

/** Create a new floating point value.
 * Returned value must be freed by caller with slv2_value_free.
 */
SLV2_API
SLV2Value
slv2_value_new_float(SLV2World world, float val);

/** Create a new boolean value.
 * Returned value must be freed by caller with slv2_value_free.
 */
SLV2_API
SLV2Value
slv2_value_new_bool(SLV2World world, bool val);

/** Free an SLV2Value.
 */
SLV2_API
void
slv2_value_free(SLV2Value val);

/** Duplicate an SLV2Value.
 */
SLV2_API
SLV2Value
slv2_value_duplicate(SLV2Value val);

/** Return whether two values are equivalent.
 */
SLV2_API
bool
slv2_value_equals(SLV2Value value, SLV2Value other);

/** Return this value as a Turtle/SPARQL token.
 * <table>
 * <caption>Example Turtle Tokens</caption>
 * <tr><th>URI</th><td>&lt;http://example.org/foo &gt;</td></tr>
 * <tr><th>QName</td><td>doap:name</td></tr>
 * <tr><th>String</td><td>"this is a string"</td></tr>
 * <tr><th>Float</td><td>1.0</td></tr>
 * <tr><th>Integer</td><td>1</td></tr>
 * <tr><th>Boolean</td><td>true</td></tr>
 * </table>
 */
SLV2_API
char*
slv2_value_get_turtle_token(SLV2Value value);

/** Return whether the value is a URI (resource).
 */
SLV2_API
bool
slv2_value_is_uri(SLV2Value value);

/** Return this value as a URI string, e.g. "http://example.org/foo".
 * Valid to call only if slv2_value_is_uri(@a value) returns true.
 * Returned value is owned by @a value and must not be freed by caller.
 */
SLV2_API
const char*
slv2_value_as_uri(SLV2Value value);

/** Return whether the value is a blank node (resource with no URI).
 */
SLV2_API
bool
slv2_value_is_blank(SLV2Value value);

/** Return this value as a blank node identifier, e.g. "genid03".
 * Valid to call only if slv2_value_is_blank(@a value) returns true.
 * Returned value is owned by @a value and must not be freed by caller.
 */
SLV2_API
const char*
slv2_value_as_blank(SLV2Value value);

/** Return whether this value is a literal (i.e. not a URI).
 * Returns true if @a value is a string or numeric value.
 */
SLV2_API
bool
slv2_value_is_literal(SLV2Value value);

/** Return whether this value is a string literal.
 * Returns true if @a value is a string (but not  numeric) value.
 */
SLV2_API
bool
slv2_value_is_string(SLV2Value value);

/** Return @a value as a string.
 */
SLV2_API
const char*
slv2_value_as_string(SLV2Value value);

/** Return whether this value is a decimal literal.
 */
SLV2_API
bool
slv2_value_is_float(SLV2Value value);

/** Return @a value as a float.
 * Valid to call only if slv2_value_is_float(@a value) or
 * slv2_value_is_int(@a value) returns true.
 */
SLV2_API
float
slv2_value_as_float(SLV2Value value);

/** Return whether this value is an integer literal.
 */
SLV2_API
bool
slv2_value_is_int(SLV2Value value);

/** Return @a value as an integer.
 * Valid to call only if slv2_value_is_int(@a value) returns true.
 */
SLV2_API
int
slv2_value_as_int(SLV2Value value);

/** Return whether this value is a boolean.
 */
SLV2_API
bool
slv2_value_is_bool(SLV2Value value);

/** Return @a value as a bool.
 * Valid to call only if slv2_value_is_bool(@a value) returns true.
 */
SLV2_API
bool
slv2_value_as_bool(SLV2Value value);

/** Convert a file URI to a local path.
 * For example, "file://foo/bar/baz.ttl" returns "/foo/bar/baz.ttl".
 * Return value is shared and must not be deleted by caller.
 * @return @a uri converted to a path, or NULL on failure (URI is not local).
 */
SLV2_API
const char* slv2_uri_to_path(const char* uri);

/** @} */
/** @name Collections
 * SLV2 has several collection types for holding various types of value:
 * <ul>
 * <li>SLV2Plugins (function prefix "slv2_plugins_")</li>
 * <li>SLV2PluginClasses (function prefix "slv2_plugin_classes_")</li>
 * <li>SLV2ScalePoints (function prefix "slv2_scale_points_")</li>
 * <li>SLV2Values (function prefix "slv2_values_")</li>
 * <li>SLV2UIs (function prefix "slv2_uis_")</li>
 * </ul>
 *
 * Each collection type supports the following functions:
 * <ul>
 * <li>PREFIX_free (coll)</li>
 * <li>PREFIX_size (coll)</li>
 * <li>PREFIX_get_at (coll, index)</li>
 * </ul>
 * @{
 */

#define SLV2_COLLECTION(CollType, ElemType, prefix) \
\
/** Free @a collection.
 */ \
SLV2_API \
void \
prefix ## _free(CollType collection); \
\
\
/** Get the number of elements in @a collection.
 */ \
SLV2_API \
unsigned \
prefix ## _size(CollType collection); \
\
\
/** Get an element from @a collection by index.
 * @a index has no significance other than as an index into this collection.
 * Any @a index not less than the size of the collection will return NULL,
 * so all elements in a collection can be enumerated by repeated calls
 * to this function starting with @a index = 0.
 * @return NULL if @a index is out of range.
 */ \
SLV2_API \
ElemType \
prefix ## _get_at(CollType collection, \
                  unsigned index);

SLV2_COLLECTION(SLV2PluginClasses, SLV2PluginClass, slv2_plugin_classes)
SLV2_COLLECTION(SLV2ScalePoints, SLV2ScalePoint, slv2_scale_points)
SLV2_COLLECTION(SLV2Values, SLV2Value, slv2_values)

/** Free a plugin collection.
 * Freeing a plugin collection does not destroy the plugins it contains
 * (plugins are owned by the world). @a plugins is invalid after this call.
 */
SLV2_API
void
slv2_plugins_free(SLV2World   world,
                  SLV2Plugins plugins);

/** Get the number of plugins in the collection.
 */
SLV2_API
unsigned
slv2_plugins_size(SLV2Plugins plugins);

/** Get a plugin from @a plugins by URI.
 * Return value is shared (stored in @a plugins) and must not be freed or
 * modified by the caller in any way.
 * @return NULL if no plugin with @a uri is found in @a plugins.
 */
SLV2_API
SLV2Plugin
slv2_plugins_get_by_uri(SLV2Plugins plugins,
                        SLV2Value   uri);

/** Get a plugin from @a plugins by index.
 * @a index has no significance other than as an index into this plugins.
 * Any @a index not less than slv2_plugins_get_length(plugins) will return NULL,
 * so all plugins in a plugins can be enumerated by repeated calls
 * to this function starting with @a index = 0.
 * @return NULL if @a index out of range.
 */
SLV2_API
SLV2Plugin
slv2_plugins_get_at(SLV2Plugins plugins,
                    unsigned    index);

/** Get a plugin class from @a classes by URI.
 * Return value is shared (stored in @a classes) and must not be freed or
 * modified by the caller in any way.
 * @return NULL if no plugin class with @a uri is found in @a classes.
 */
SLV2_API
SLV2PluginClass
slv2_plugin_classes_get_by_uri(SLV2PluginClasses classes,
                               SLV2Value         uri);

/** Allocate a new, empty SLV2ScalePoints.
 */
SLV2_API
SLV2ScalePoints
slv2_scale_points_new(void);

/** Allocate a new, empty SLV2Values.
 */
SLV2_API
SLV2Values
slv2_values_new(void);

/** Return whether @a values contains @a value.
 */
SLV2_API
bool
slv2_values_contains(SLV2Values values, SLV2Value value);

/** @} */
/** @name World
 * The "world" represents all SLV2 state, and is used to discover/load/cache
 * LV2 data (plugins, UIs, and extensions).
 * Normal hosts which just need to load plugins by URI should simply use
 * @ref slv2_world_load_all to discover/load the system's LV2 resources.
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
/** @name Plugin
 * @{
 */

/** Check if @a plugin is valid.
 * This is not a rigorous validator, but can be used to reject some malformed
 * plugins that could cause bugs (e.g. plugins with missing required fields).
 *
 * Note that normal hosts do NOT need to use this - slv2 does not
 * load invalid plugins into plugin lists.  This is included for plugin
 * testing utilities, etc.
 * @return true iff @a plugin is valid.
 */
SLV2_API
bool
slv2_plugin_verify(SLV2Plugin plugin);

/** Get the URI of @a plugin.
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
 * @return A shared URI value which must not be modified or freed.
 */
SLV2_API
SLV2Value
slv2_plugin_get_uri(SLV2Plugin plugin);

/** Get the (resolvable) URI of the plugin's "main" bundle.
 * This returns the URI of the bundle where the plugin itself was found.
 * Note that the data for a plugin may be spread over many bundles, that is,
 * slv2_plugin_get_data_uris may return URIs which are not within this bundle.
 *
 * Typical hosts should not need to use this function.
 * Note this always returns a fully qualified URI.  If you want a local
 * filesystem path, use slv2_uri_to_path.
 * @return a shared string which must not be modified or freed.
 */
SLV2_API
SLV2Value
slv2_plugin_get_bundle_uri(SLV2Plugin plugin);

/** Get the (resolvable) URIs of the RDF data files that define a plugin.
 * Typical hosts should not need to use this function.
 * Note this always returns fully qualified URIs.  If you want local
 * filesystem paths, use slv2_uri_to_path.
 * @return a list of complete URLs eg. "file:///foo/ABundle.lv2/aplug.ttl",
 * which is shared and must not be modified or freed.
 */
SLV2_API
SLV2Values
slv2_plugin_get_data_uris(SLV2Plugin plugin);

/** Get the (resolvable) URI of the shared library for @a plugin.
 * Note this always returns a fully qualified URI.  If you want a local
 * filesystem path, use slv2_uri_to_path.
 * @return a shared string which must not be modified or freed.
 */
SLV2_API
SLV2Value
slv2_plugin_get_library_uri(SLV2Plugin plugin);

/** Get the name of @a plugin.
 * This returns the name (doap:name) of the plugin.  The name may be
 * translated according to the current locale, this value MUST NOT be used
 * as a plugin identifier (use the URI for that).
 * Returned value must be freed by the caller.
 */
SLV2_API
SLV2Value
slv2_plugin_get_name(SLV2Plugin plugin);

/** Get the class this plugin belongs to (ie Filters).
 */
SLV2_API
SLV2PluginClass
slv2_plugin_get_class(SLV2Plugin plugin);

/** Get a value associated with the plugin in a plugin's data files.
 * @a predicate must be either a URI or a QName.
 *
 * Returns the ?object of all triples found of the form:
 *
 * <code>&lt;plugin-uri&gt; predicate ?object</code>
 *
 * May return NULL if the property was not found, or if object(s) is not
 * sensibly represented as an SLV2Values (e.g. blank nodes).
 * Return value must be freed by caller with slv2_values_free.
 */
SLV2_API
SLV2Values
slv2_plugin_get_value(SLV2Plugin p,
                      SLV2Value  predicate);

/** Get a value associated with the plugin in a plugin's data files.
 * This function is identical to slv2_plugin_get_value, but takes a QName
 * string parameter for a predicate instead of an SLV2Value, which may be
 * more convenient.
 */
SLV2_API
SLV2Values
slv2_plugin_get_value_by_qname(SLV2Plugin  p,
                               const char* predicate);

/** Get a value associated with some subject in a plugin's data files.
 * @a predicate must be either a URI or a QName.
 *
 * Returns the ?object of all triples found of the form:
 *
 * <code>subject predicate ?object</code>
 *
 * This can be used to investigate URIs returned by slv2_plugin_get_value
 * (if information about it is contained in the plugin's data files).
 *
 * May return NULL if the property was not found, or if object is not
 * sensibly represented as an SLV2Values (e.g. blank nodes).
 * Return value must be freed by caller with slv2_values_free.
 */
SLV2_API
SLV2Values
slv2_plugin_get_value_for_subject(SLV2Plugin p,
                                  SLV2Value  subject_uri,
                                  SLV2Value  predicate_uri);

/** Return whether a feature is supported by a plugin.
 * This will return true if the feature is an optional or required feature
 * of the plugin.
 */
SLV2_API
bool
slv2_plugin_has_feature(SLV2Plugin p,
                        SLV2Value  feature_uri);

/** Get the LV2 Features supported (required or optionally) by a plugin.
 * A feature is "supported" by a plugin if it is required OR optional.
 *
 * Since required features have special rules the host must obey, this function
 * probably shouldn't be used by normal hosts.  Using slv2_plugin_get_optional_features
 * and slv2_plugin_get_required_features separately is best in most cases.
 *
 * Returned value must be freed by caller with slv2_values_free.
 */
SLV2_API
SLV2Values
slv2_plugin_get_supported_features(SLV2Plugin p);

/** Get the LV2 Features required by a plugin.
 * If a feature is required by a plugin, hosts MUST NOT use the plugin if they do not
 * understand (or are unable to support) that feature.
 *
 * All values returned here MUST be passed to the plugin's instantiate method
 * (along with data, if necessary, as defined by the feature specification)
 * or plugin instantiation will fail.
 *
 * Return value must be freed by caller with slv2_values_free.
 */
SLV2_API
SLV2Values
slv2_plugin_get_required_features(SLV2Plugin p);

/** Get the LV2 Features optionally supported by a plugin.
 * Hosts MAY ignore optional plugin features for whatever reasons.  Plugins
 * MUST operate (at least somewhat) if they are instantiated without being
 * passed optional features.
 *
 * Return value must be freed by caller with slv2_values_free.
 */
SLV2_API
SLV2Values
slv2_plugin_get_optional_features(SLV2Plugin p);

/** Get the number of ports on this plugin.
 */
SLV2_API
uint32_t
slv2_plugin_get_num_ports(SLV2Plugin p);

/** Get the port ranges (minimum, maximum and default values) for all ports.
 * @a min_values, @a max_values and @a def_values must either point to an array
 * of N floats, where N is the value returned by slv2_plugin_get_num_ports()
 * for this plugin, or NULL.  The elements of the array will be set to the
 * the minimum, maximum and default values of the ports on this plugin,
 * with array index corresponding to port index.  If a port doesn't have a
 * minimum, maximum or default value, or the port's type is not float, the
 * corresponding array element will be set to NAN.
 *
 * This is a convenience method for the common case of getting the range of
 * all float ports on a plugin, and may be significantly faster than
 * repeated calls to slv2_port_get_range.
 */
SLV2_API
void
slv2_plugin_get_port_ranges_float(SLV2Plugin p,
                                  float*     min_values,
                                  float*     max_values,
                                  float*     def_values);

/** Get the number of ports on this plugin that are members of some class(es).
 * Note that this is a varargs function so ports fitting any type 'profile'
 * desired can be found quickly.  REMEMBER TO TERMINATE THE PARAMETER LIST
 * OF THIS FUNCTION WITH NULL OR VERY NASTY THINGS WILL HAPPEN.
 */
SLV2_API
uint32_t
slv2_plugin_get_num_ports_of_class(SLV2Plugin p,
                                   SLV2Value  class_1, ...);

/** Return whether or not the plugin introduces (and reports) latency.
 * The index of the latency port can be found with slv2_plugin_get_latency_port
 * ONLY if this function returns true.
 */
SLV2_API
bool
slv2_plugin_has_latency(SLV2Plugin p);

/** Return the index of the plugin's latency port.
 * It is a fatal error to call this on a plugin without checking if the port
 * exists by first calling slv2_plugin_has_latency.
 *
 * Any plugin that introduces unwanted latency that should be compensated for
 * (by hosts with the ability/need) MUST provide this port, which is a control
 * rate output port that reports the latency for each cycle in frames.
 */
SLV2_API
uint32_t
slv2_plugin_get_latency_port_index(SLV2Plugin p);

/** Get a port on @a plugin by @a index.
 */
SLV2_API
SLV2Port
slv2_plugin_get_port_by_index(SLV2Plugin plugin,
                              uint32_t   index);

/** Get a port on @a plugin by @a symbol.
 * Note this function is slower than slv2_plugin_get_port_by_index,
 * especially on plugins with a very large number of ports.
 */
SLV2_API
SLV2Port
slv2_plugin_get_port_by_symbol(SLV2Plugin plugin,
                               SLV2Value  symbol);

/** Get the full name of the plugin's author.
 * Returns NULL if author name is not present.
 * Returned value must be freed by caller.
 */
SLV2_API
SLV2Value
slv2_plugin_get_author_name(SLV2Plugin plugin);

/** Get the email address of the plugin's author.
 * Returns NULL if author email address is not present.
 * Returned value must be freed by caller.
 */
SLV2_API
SLV2Value
slv2_plugin_get_author_email(SLV2Plugin plugin);

/** Get the email address of the plugin's author.
 * Returns NULL if author homepage is not present.
 * Returned value must be freed by caller.
 */
SLV2_API
SLV2Value
slv2_plugin_get_author_homepage(SLV2Plugin plugin);

/** @} */
/** @name Port
 * @{
 */

/** Port analog of slv2_plugin_get_value.
 */
SLV2_API
SLV2Values
slv2_port_get_value(SLV2Plugin plugin,
                    SLV2Port   port,
                    SLV2Value  predicate);

/** Port analog of slv2_plugin_get_value_by_qname.
 */
SLV2_API
SLV2Values
slv2_port_get_value_by_qname(SLV2Plugin  plugin,
                             SLV2Port    port,
                             const char* property_uri);

/** Return the LV2 port properties of a port.
 */
SLV2_API
SLV2Values
slv2_port_get_properties(SLV2Plugin plugin,
                         SLV2Port   port);

/** Return whether a port has a certain property.
 */
SLV2_API
bool
slv2_port_has_property(SLV2Plugin p,
                       SLV2Port   port,
                       SLV2Value  property_uri);

/** Return whether a port is an event port and supports a certain event type.
 */
SLV2_API
bool
slv2_port_supports_event(SLV2Plugin p,
                         SLV2Port   port,
                         SLV2Value  event_uri);

/** Get the symbol of a port.
 * The 'symbol' is a short string, a valid C identifier.
 * Returned value is owned by @a port and must not be freed.
 */
SLV2_API
SLV2Value
slv2_port_get_symbol(SLV2Plugin plugin,
                     SLV2Port   port);

/** Get the name of a port.
 * This is guaranteed to return the untranslated name (the doap:name in the
 * data file without a language tag).  Returned value must be freed by
 * the caller.
 */
SLV2_API
SLV2Value
slv2_port_get_name(SLV2Plugin plugin,
                   SLV2Port   port);

/** Get all the classes of a port.
 * This can be used to determine if a port is an input, output, audio,
 * control, midi, etc, etc, though it's simpler to use slv2_port_is_a.
 * The returned list does not include lv2:Port, which is implied.
 * Returned value is shared and must not be destroyed by caller.
 */
SLV2_API
SLV2Values
slv2_port_get_classes(SLV2Plugin plugin,
                      SLV2Port   port);

/** Determine if a port is of a given class (input, output, audio, etc).
 * For convenience/performance/extensibility reasons, hosts are expected to
 * create an SLV2Value for each port class they "care about".  Well-known type
 * URI strings are defined (e.g. SLV2_PORT_CLASS_INPUT) for convenience, but
 * this function is designed so that SLV2 is usable with any port types
 * without requiring explicit support in SLV2.
 */
SLV2_API
bool
slv2_port_is_a(SLV2Plugin plugin,
               SLV2Port   port,
               SLV2Value  port_class);

/** Get the default, minimum, and maximum values of a port.
 * @a def, @a min, and @a max are outputs, pass pointers to uninitialized
 * (i.e. NOT created with slv2_value_new) SLV2Value variables.  These will
 * be set to point at new values (which must be freed by the caller using
 * slv2_value_free), or NULL if the value does not exist.
 */
SLV2_API
void
slv2_port_get_range(SLV2Plugin plugin,
                    SLV2Port   port,
                    SLV2Value* deflt,
                    SLV2Value* min,
                    SLV2Value* max);

/** Get the scale points (enumeration values) of a port.
 * This returns a collection of 'interesting' named values of a port
 * (e.g. appropriate entries for a UI selector associated with this port).
 * Returned value may be NULL if @a port has no scale points, otherwise it
 * must be freed by caller with slv2_scale_points_free.
 */
SLV2_API
SLV2ScalePoints
slv2_port_get_scale_points(SLV2Plugin plugin,
                           SLV2Port   port);


/** @} */
/** @name Scale Point
 * @{
 */

/** Get the label of this scale point (enumeration value)
 * Returned value is owned by @a point and must not be freed.
 */
SLV2_API
SLV2Value
slv2_scale_point_get_label(SLV2ScalePoint point);

/** Get the value of this scale point (enumeration value)
 * Returned value is owned by @a point and must not be freed.
 */
SLV2_API
SLV2Value
slv2_scale_point_get_value(SLV2ScalePoint point);

/** @} */
/** @name Plugin Class
 * @{
 */

/** Get the URI of this class' superclass.
 * Returned value is owned by @a plugin_class and must not be freed by caller.
 * Returned value may be NULL, if class has no parent.
 */
SLV2_API
SLV2Value
slv2_plugin_class_get_parent_uri(SLV2PluginClass plugin_class);

/** Get the URI of this plugin class.
 * Returned value is owned by @a plugin_class and must not be freed by caller.
 */
SLV2_API
SLV2Value
slv2_plugin_class_get_uri(SLV2PluginClass plugin_class);

/** Get the label of this plugin class, ie "Oscillators".
 * Returned value is owned by @a plugin_class and must not be freed by caller.
 */
SLV2_API
SLV2Value slv2_plugin_class_get_label(SLV2PluginClass plugin_class);

/** Get the subclasses of this plugin class.
 * Returned value must be freed by caller with slv2_plugin_classes_free.
 */
SLV2_API
SLV2PluginClasses
slv2_plugin_class_get_children(SLV2PluginClass plugin_class);

/** @} */
/** @name Plugin Instance
 * @{
 */

typedef struct _SLV2InstanceImpl* SLV2InstanceImpl;

/* Instance of a plugin.
 * This is exposed in the ABI to allow inlining of performance critical
 * functions like slv2_instance_run (simple wrappers of functions in lv2.h).
 * This is for performance reasons, user code should not use this definition
 * in any way (which is why it is not machine documented).
 * Truly private implementation details are hidden via @a ref pimpl.
 */
typedef struct _Instance {
	const LV2_Descriptor* lv2_descriptor;
	LV2_Handle            lv2_handle;
	SLV2InstanceImpl      pimpl; ///< Private implementation
}* SLV2Instance;

/** Instantiate a plugin.
 * The returned value is a lightweight handle for an LV2 plugin instance,
 * it does not refer to @a plugin, or any other SLV2 state.  The caller must
 * eventually free it with slv2_instance_free.
 * @a features is a NULL-terminated array of features the host supports.
 * NULL may be passed if the host supports no additional features.
 * @return NULL if instantiation failed.
 */
SLV2_API
SLV2Instance
slv2_plugin_instantiate(SLV2Plugin               plugin,
                        double                   sample_rate,
                        const LV2_Feature*const* features);

/** Free a plugin instance.
 * @a instance is invalid after this call.
 */
SLV2_API
void
slv2_instance_free(SLV2Instance instance);

#ifndef SLV2_INTERNAL

/** Get the URI of the plugin which @a instance is an instance of.
 * Returned string is shared and must not be modified or deleted.
 */
static inline const char*
slv2_instance_get_uri(SLV2Instance instance)
{
	return instance->lv2_descriptor->URI;
}

/** Connect a port to a data location.
 * This may be called regardless of whether the plugin is activated,
 * activation and deactivation does not destroy port connections.
 */
static inline void
slv2_instance_connect_port(SLV2Instance instance,
                           uint32_t     port_index,
                           void*        data_location)
{
	instance->lv2_descriptor->connect_port
		(instance->lv2_handle, port_index, data_location);
}

/** Activate a plugin instance.
 * This resets all state information in the plugin, except for port data
 * locations (as set by slv2_instance_connect_port).  This MUST be called
 * before calling slv2_instance_run.
 */
static inline void
slv2_instance_activate(SLV2Instance instance)
{
	if (instance->lv2_descriptor->activate)
		instance->lv2_descriptor->activate(instance->lv2_handle);
}

/** Run @a instance for @a sample_count frames.
 * If the hint lv2:hardRTCapable is set for this plugin, this function is
 * guaranteed not to block.
 */
static inline void
slv2_instance_run(SLV2Instance instance,
                  uint32_t     sample_count)
{
	instance->lv2_descriptor->run(instance->lv2_handle, sample_count);
}

/** Deactivate a plugin instance.
 * Note that to run the plugin after this you must activate it, which will
 * reset all state information (except port connections).
 */
static inline void
slv2_instance_deactivate(SLV2Instance instance)
{
	if (instance->lv2_descriptor->deactivate)
		instance->lv2_descriptor->deactivate(instance->lv2_handle);
}

/** Get extension data from the plugin instance.
 * The type and semantics of the data returned is specific to the particular
 * extension, though in all cases it is shared and must not be deleted.
 */
static inline const void*
slv2_instance_get_extension_data(SLV2Instance instance,
                                 const char*  uri)
{
	if (instance->lv2_descriptor->extension_data)
		return instance->lv2_descriptor->extension_data(uri);
	else
		return NULL;
}

/** Get the LV2_Descriptor of the plugin instance.
 * Normally hosts should not need to access the LV2_Descriptor directly,
 * use the slv2_instance_* functions.
 *
 * The returned descriptor is shared and must not be deleted.
 */
static inline const LV2_Descriptor*
slv2_instance_get_descriptor(SLV2Instance instance)
{
	return instance->lv2_descriptor;
}

/** Get the LV2_Handle of the plugin instance.
 * Normally hosts should not need to access the LV2_Handle directly,
 * use the slv2_instance_* functions.
 *
 * The returned handle is shared and must not be deleted.
 */
static inline LV2_Handle
slv2_instance_get_handle(SLV2Instance instance)
{
	return instance->lv2_handle;
}

#endif /* SLV2_INTERNAL */

/** @} */
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SLV2_SLV2_H__ */
