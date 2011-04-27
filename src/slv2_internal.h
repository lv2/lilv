/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef SLV2_INTERNAL_H
#define SLV2_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __WIN32__
#include <windows.h>
#define dlopen(path, flags) LoadLibrary(path)
#define dlclose(lib) FreeLibrary(lib)
#define dlsym GetProcAddress
static inline char* dlerror(void) { return "Unknown error"; }
#else
#include <dlfcn.h>
#endif

#include <glib.h>

#include "serd/serd.h"
#include "sord/sord.h"

#include "slv2-config.h"

#ifdef SLV2_DYN_MANIFEST
#include "lv2/lv2plug.in/ns/ext/dyn-manifest/dyn-manifest.h"
#endif

#ifdef HAVE_SUIL
#include "suil/suil.h"
#endif

#include "slv2/slv2.h"

#define SLV2_NS_DOAP (const uint8_t*)"http://usefulinc.com/ns/doap#"
#define SLV2_NS_RDFS (const uint8_t*)"http://www.w3.org/2000/01/rdf-schema#"
#define SLV2_NS_SLV2 (const uint8_t*)"http://drobilla.net/ns/slv2#"
#define SLV2_NS_LV2  (const uint8_t*)"http://lv2plug.in/ns/lv2core#"
#define SLV2_NS_XSD  (const uint8_t*)"http://www.w3.org/2001/XMLSchema#"
#define SLV2_NS_RDF  (const uint8_t*)"http://www.w3.org/1999/02/22-rdf-syntax-ns#"

typedef SordIter SLV2Matches;
typedef SordNode SLV2Node;

#define FOREACH_MATCH(iter) \
	for (; !sord_iter_end(iter); sord_iter_next(iter))

static inline SLV2Node
slv2_match_subject(SLV2Matches iter) {
	SordQuad tup;
	sord_iter_get(iter, tup);
	return tup[SORD_SUBJECT];
}

static inline SLV2Node
slv2_match_object(SLV2Matches iter) {
	SordQuad tup;
	sord_iter_get(iter, tup);
	return tup[SORD_OBJECT];
}

static inline void
slv2_match_end(SLV2Matches iter)
{
	sord_iter_free(iter);
}

/* ********* PORT ********* */

/** Reference to a port on some plugin. */
struct _SLV2Port {
	uint32_t   index;   ///< lv2:index
	SLV2Value  symbol;  ///< lv2:symbol
	SLV2Values classes; ///< rdf:type
};

SLV2Port slv2_port_new(SLV2World world, uint32_t index, const char* symbol);
void     slv2_port_free(SLV2Port port);

/* ********* Plugin ********* */

/** Header of an SLV2Plugin, SLV2PluginClass, or SLV2UI.
 * Any of these structs may be safely casted to _SLV2Header, which is used to
 * implement sequences without code duplication (see slv2_sequence_get_by_uri).
 */
struct _SLV2Header {
	struct _SLV2World* world;
	SLV2Value          uri;
};

/** Record of an installed/available plugin.
 *
 * A simple reference to a plugin somewhere on the system. This just holds
 * paths of relevant files, the actual data therein isn't loaded into memory.
 */
struct _SLV2Plugin {
	struct _SLV2World* world;
	SLV2Value          plugin_uri;
	SLV2Value          bundle_uri; ///< Bundle directory plugin was loaded from
	SLV2Value          binary_uri; ///< lv2:binary
	SLV2Value          dynman_uri; ///< dynamic manifest binary
	SLV2PluginClass    plugin_class;
	SLV2Values         data_uris; ///< rdfs::seeAlso
	SLV2Port*          ports;
	uint32_t           num_ports;
	bool               loaded;
	bool               replaced;
};

SLV2Plugin slv2_plugin_new(SLV2World world, SLV2Value uri, SLV2Value bundle_uri);
void       slv2_plugin_load_if_necessary(SLV2Plugin p);
void       slv2_plugin_free(SLV2Plugin plugin);

SLV2Value
slv2_plugin_get_unique(SLV2Plugin p,
                       SLV2Node   subject,
                       SLV2Node   predicate);

/* ********* Plugins ********* */

SLV2Plugins
slv2_plugins_new();

/**
   Free @a collection.
*/
SLV2_API
void
slv2_collection_free(SLV2Collection collection);

/**
   Get the number of elements in @a collection.
*/
SLV2_API
unsigned
slv2_collection_size(SLV2Collection collection);

/**
   Get an element from @a collection by index.

   @a index has no significance other than as an index into @a collection.

   Any @a index not less than the size of the collection will return NULL,
   so all elements in a collection can be enumerated by repeated calls
   to this function starting with @a index = 0.

   Note this function is a search, and not constant time.
   This function is deprecated, use iterators instead.

   @return NULL if @a index is out of range.
*/
SLV2_DEPRECATED
SLV2_API
void*
slv2_collection_get_at(SLV2Collection collection,
                       unsigned       index);

/* ********* Instance ********* */

/** Pimpl portion of SLV2Instance */
struct _SLV2InstanceImpl {
	void* lib_handle;
};

/* ********* Plugin Class ********* */

struct _SLV2PluginClass {
	struct _SLV2World* world;
	SLV2Value          uri;
	SLV2Value          parent_uri;
	SLV2Value          label;
};

SLV2PluginClass slv2_plugin_class_new(SLV2World   world,
                                      SLV2Node    parent_uri,
                                      SLV2Node    uri,
                                      const char* label);

void slv2_plugin_class_free(SLV2PluginClass plugin_class);

/* ********* Plugin Classes ********* */

SLV2PluginClasses slv2_plugin_classes_new();
void              slv2_plugin_classes_free();

/* ********* World ********* */

typedef struct {
	bool dyn_manifest;
	bool filter_language;
} SLV2Options;

/** Model of LV2 (RDF) data loaded from bundles.
 */
struct _SLV2World {
	SordWorld         world;
	SordModel         model;
	SerdReader*       reader;
	SerdEnv*          namespaces;
	unsigned          n_read_files;
	SLV2PluginClass   lv2_plugin_class;
	SLV2PluginClasses plugin_classes;
	SLV2Plugins       plugins;
	SLV2Node          dc_replaces_node;
	SLV2Node          dyn_manifest_node;
	SLV2Node          lv2_specification_node;
	SLV2Node          lv2_plugin_node;
	SLV2Node          lv2_binary_node;
	SLV2Node          lv2_default_node;
	SLV2Node          lv2_minimum_node;
	SLV2Node          lv2_maximum_node;
	SLV2Node          lv2_port_node;
	SLV2Node          lv2_portproperty_node;
	SLV2Node          lv2_reportslatency_node;
	SLV2Node          lv2_index_node;
	SLV2Node          lv2_symbol_node;
	SLV2Node          rdf_a_node;
	SLV2Node          rdf_value_node;
	SLV2Node          rdfs_class_node;
	SLV2Node          rdfs_label_node;
	SLV2Node          rdfs_seealso_node;
	SLV2Node          rdfs_subclassof_node;
	SLV2Node          slv2_bundleuri_node;
	SLV2Node          slv2_dmanifest_node;
	SLV2Node          xsd_boolean_node;
	SLV2Node          xsd_decimal_node;
	SLV2Node          xsd_double_node;
	SLV2Node          xsd_integer_node;
	SLV2Value         doap_name_val;
	SLV2Value         lv2_name_val;
	SLV2Options       opt;
};

const uint8_t*
slv2_world_blank_node_prefix(SLV2World world);
void
slv2_world_load_file(SLV2World world, const char* file_uri);

/* ********* Plugin UI ********* */

struct _SLV2UI {
	struct _SLV2World* world;
	SLV2Value          uri;
	SLV2Value          bundle_uri;
	SLV2Value          binary_uri;
	SLV2Values         classes;
};

SLV2UIs slv2_uis_new();

SLV2UI
slv2_ui_new(SLV2World world,
            SLV2Value uri,
            SLV2Value type_uri,
            SLV2Value binary_uri);

void slv2_ui_free(SLV2UI ui);

/* ********* Value ********* */

typedef enum _SLV2ValueType {
	SLV2_VALUE_URI,
	SLV2_VALUE_QNAME_UNUSED, ///< FIXME: APIBREAK: remove
	SLV2_VALUE_STRING,
	SLV2_VALUE_INT,
	SLV2_VALUE_FLOAT,
	SLV2_VALUE_BOOL,
	SLV2_VALUE_BLANK
} SLV2ValueType;

struct _SLV2Value {
	SLV2ValueType type;
	char*         str_val; ///< always present
	union {
		int       int_val;
		float     float_val;
		bool      bool_val;
		SLV2Node  uri_val;
	} val;
};

SLV2Value slv2_value_new(SLV2World world, SLV2ValueType type, const char* val);
SLV2Value slv2_value_new_from_node(SLV2World world, SLV2Node node);
SLV2Node  slv2_value_as_node(SLV2Value value);

int
slv2_header_compare_by_uri(const void* a, const void* b, void* user_data);

static inline void
slv2_sequence_insert(GSequence* seq, void* value)
{
	g_sequence_insert_sorted(seq, value, slv2_header_compare_by_uri, NULL);
}

static inline void
slv2_array_append(GSequence* seq, void* value) {
	g_sequence_append(seq, value);
}

struct _SLV2Header*
slv2_sequence_get_by_uri(GSequence* seq, SLV2Value uri);

static inline SLV2Node slv2_node_copy(SLV2Node node) {
	return sord_node_copy(node);
}

static inline void slv2_node_free(SLV2World world, SLV2Node node) {
	sord_node_free(world->world, node);
}

/* ********* Scale Points ********* */

struct _SLV2ScalePoint {
	SLV2Value value;
	SLV2Value label;
};

SLV2ScalePoint slv2_scale_point_new(SLV2Value value, SLV2Value label);
void           slv2_scale_point_free(SLV2ScalePoint point);

/* ********* Query Results ********* */

SLV2Matches slv2_plugin_find_statements(SLV2Plugin plugin,
                                        SLV2Node   subject,
                                        SLV2Node   predicate,
                                        SLV2Node   object);

static inline bool slv2_matches_next(SLV2Matches matches) {
	return sord_iter_next(matches);
}

static inline bool slv2_matches_end(SLV2Matches matches) {
	return sord_iter_end(matches);
}

SLV2Values slv2_values_from_stream_objects(SLV2Plugin  p,
                                           SLV2Matches stream);

/* ********* Utilities ********* */

char*    slv2_strjoin(const char* first, ...);
char*    slv2_strdup(const char* str);
char*    slv2_get_lang();
uint8_t* slv2_qname_expand(SLV2Plugin p, const char* qname);

typedef void (*VoidFunc)();

/** dlsym wrapper to return a function pointer (without annoying warning) */
static inline VoidFunc
slv2_dlfunc(void* handle, const char* symbol)
{
	typedef VoidFunc (*VoidFuncGetter)(void*, const char*);
	VoidFuncGetter dlfunc = (VoidFuncGetter)dlsym;
	return dlfunc(handle, symbol);
}

/* ********* Dynamic Manifest ********* */
#ifdef SLV2_DYN_MANIFEST
static const LV2_Feature* const dman_features = { NULL };
#endif

#define SLV2_ERROR(str)       fprintf(stderr, "ERROR: %s: " str, __func__)
#define SLV2_ERRORF(fmt, ...) fprintf(stderr, "ERROR: %s: " fmt, __func__, __VA_ARGS__)

#define SLV2_WARN(str) fprintf(stderr, "WARNING: %s: " str, __func__)
#define SLV2_WARNF(fmt, ...) fprintf(stderr, "WARNING: %s: " fmt, __func__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* SLV2_INTERNAL_H */
