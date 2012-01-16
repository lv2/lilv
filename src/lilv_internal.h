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

#ifndef LILV_INTERNAL_H
#define LILV_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
#    include <windows.h>
#    define dlopen(path, flags) LoadLibrary(path)
#    define dlclose(lib) FreeLibrary(lib)
#    define dlsym GetProcAddress
static inline char* dlerror(void) { return "Unknown error"; }
#else
#    include <dlfcn.h>
#endif

#include "serd/serd.h"
#include "sord/sord.h"

#include "zix/tree.h"

#include "lilv-config.h"
#include "lilv/lilv.h"

#ifdef LILV_DYN_MANIFEST
#    include "lv2/lv2plug.in/ns/ext/dynmanifest/dynmanifest.h"
#endif

/*
 *
 * Types
 *
 */

typedef struct LilvSpecImpl LilvSpec;

typedef void LilvCollection;

struct LilvPortImpl {
	SordNode*  node;     ///< RDF node
	uint32_t   index;    ///< lv2:index
	LilvNode*  symbol;   ///< lv2:symbol
	LilvNodes* classes;  ///< rdf:type
};

struct LilvSpecImpl {
	SordNode*            spec;
	SordNode*            bundle;
	LilvNodes*           data_uris;
	struct LilvSpecImpl* next;
};

/**
   Header of an LilvPlugin, LilvPluginClass, or LilvUI.
   Any of these structs may be safely casted to LilvHeader, which is used to
   implement collections using the same comparator.
*/
struct LilvHeader {
	LilvWorld* world;
	LilvNode*  uri;
};

#ifdef LILV_DYN_MANIFEST
typedef struct {
	LilvNode*               bundle;
	void*                   lib;
	LV2_Dyn_Manifest_Handle handle;
	uint32_t                refs;
} LilvDynManifest;
#endif

struct LilvPluginImpl {
	LilvWorld*             world;
	LilvNode*              plugin_uri;
	LilvNode*              bundle_uri;  ///< Bundle plugin was loaded from
	LilvNode*              binary_uri;  ///< lv2:binary
#ifdef LILV_DYN_MANIFEST
	LilvDynManifest*       dynmanifest;
#endif
	const LilvPluginClass* plugin_class;
	LilvNodes*             data_uris;  ///< rdfs::seeAlso
	LilvPort**             ports;
	uint32_t               num_ports;
	bool                   loaded;
	bool                   replaced;
};

struct LilvPluginClassImpl {
	LilvWorld* world;
	LilvNode*  uri;
	LilvNode*  parent_uri;
	LilvNode*  label;
};

typedef struct {
	bool dyn_manifest;
	bool filter_language;
} LilvOptions;

struct LilvWorldImpl {
	SordWorld*         world;
	SordModel*         model;
	SerdReader*        reader;
	unsigned           n_read_files;
	LilvPluginClass*   lv2_plugin_class;
	LilvPluginClasses* plugin_classes;
	LilvSpec*          specs;
	LilvPlugins*       plugins;
	LilvNodes*         loaded_files;
	struct {
		SordNode* dc_replaces;
		SordNode* doap_name;
		SordNode* dman_DynManifest;
		SordNode* lv2_appliesTo;
		SordNode* lv2_binary;
		SordNode* lv2_default;
		SordNode* lv2_extensionData;
		SordNode* lv2_index;
		SordNode* lv2_maximum;
		SordNode* lv2_minimum;
		SordNode* lv2_name;
		SordNode* lv2_optionalFeature;
		SordNode* lv2_Plugin;
		SordNode* lv2_port;
		SordNode* lv2_portProperty;
		SordNode* lv2_reportsLatency;
		SordNode* lv2_requiredFeature;
		SordNode* lv2_Specification;
		SordNode* lv2_symbol;
		SordNode* pset_value;
		SordNode* rdf_a;
		SordNode* rdf_value;
		SordNode* rdfs_Class;
		SordNode* rdfs_label;
		SordNode* rdfs_seeAlso;
		SordNode* rdfs_subClassOf;
		SordNode* xsd_base64Binary;
		SordNode* xsd_boolean;
		SordNode* xsd_decimal;
		SordNode* xsd_double;
		SordNode* xsd_integer;
		SordNode* null_uri;
	} uris;
	LilvOptions opt;
};

typedef enum {
	LILV_VALUE_URI,
	LILV_VALUE_STRING,
	LILV_VALUE_INT,
	LILV_VALUE_FLOAT,
	LILV_VALUE_BOOL,
	LILV_VALUE_BLANK,
	LILV_VALUE_BLOB
} LilvNodeType;

struct LilvNodeImpl {
	LilvWorld*    world;
	char*         str_val;  ///< always present
	union {
		int       int_val;
		float     float_val;
		bool      bool_val;
		SordNode* uri_val;
		struct {
			void*  buf;
			size_t size;
		} blob_val;
	} val;
	LilvNodeType  type;
};

struct LilvScalePointImpl {
	LilvNode* value;
	LilvNode* label;
};

struct LilvUIImpl {
	LilvWorld* world;
	LilvNode*  uri;
	LilvNode*  bundle_uri;
	LilvNode*  binary_uri;
	LilvNodes* classes;
};

/*
 *
 * Functions
 *
 */

LilvPort* lilv_port_new(LilvWorld*      world,
                        const SordNode* node,
                        uint32_t        index,
                        const char*     symbol);
void      lilv_port_free(const LilvPlugin* plugin, LilvPort* port);

LilvPlugin* lilv_plugin_new(LilvWorld* world,
                            LilvNode*  uri,
                            LilvNode*  bundle_uri);
void        lilv_plugin_load_if_necessary(const LilvPlugin* p);
void        lilv_plugin_free(LilvPlugin* plugin);
LilvNode*   lilv_plugin_get_unique(const LilvPlugin* p,
                                   const SordNode*   subject,
                                   const SordNode*   predicate);

void      lilv_collection_free(LilvCollection* collection);
unsigned  lilv_collection_size(const LilvCollection* collection);
LilvIter* lilv_collection_begin(const LilvCollection* collection);
void*     lilv_collection_get(const LilvCollection* collection,
                              const LilvIter*       i);

LilvPluginClass* lilv_plugin_class_new(LilvWorld*      world,
                                       const SordNode* parent_uri,
                                       const SordNode* uri,
                                       const char*     label);

void lilv_plugin_class_free(LilvPluginClass* plugin_class);

LilvNodes*         lilv_nodes_new(void);
LilvPlugins*       lilv_plugins_new(void);
LilvScalePoints*   lilv_scale_points_new(void);
LilvPluginClasses* lilv_plugin_classes_new(void);
LilvUIs*           lilv_uis_new(void);

const uint8_t* lilv_world_blank_node_prefix(LilvWorld* world);
void           lilv_world_load_file(LilvWorld* world, const char* file_uri);

LilvUI* lilv_ui_new(LilvWorld* world,
                    LilvNode* uri,
                    LilvNode* type_uri,
                    LilvNode* binary_uri);

void lilv_ui_free(LilvUI* ui);

LilvNode*       lilv_node_new(LilvWorld*   world,
                              LilvNodeType type,
                              const char*  val);
LilvNode*       lilv_node_new_from_node(LilvWorld*      world,
                                        const SordNode* node);
const SordNode* lilv_node_as_node(const LilvNode* value);

int lilv_header_compare_by_uri(const void* a, const void* b, void* user_data);

int
lilv_ptr_cmp(const void* a, const void* b, void* user_data);

int
lilv_resource_node_cmp(const void* a, const void* b, void* user_data);

struct LilvHeader*
lilv_collection_get_by_uri(const ZixTree* seq, const LilvNode* uri);

LilvScalePoint* lilv_scale_point_new(LilvNode* value, LilvNode* label);
void            lilv_scale_point_free(LilvScalePoint* point);

SordIter*
lilv_world_query_internal(LilvWorld*      world,
                          const SordNode* subject,
                          const SordNode* predicate,
                          const SordNode* object);

LilvNodes*
lilv_world_query_values_internal(LilvWorld*      world,
                                 const SordNode* subject,
                                 const SordNode* predicate,
                                 const SordNode* object);

#define FOREACH_MATCH(iter) \
	for (; !sord_iter_end(iter); sord_iter_next(iter))

static inline const SordNode*
lilv_match_subject(SordIter* iter) {
	SordQuad tup;
	sord_iter_get(iter, tup);
	return tup[SORD_SUBJECT];
}

static inline const SordNode*
lilv_match_predicate(SordIter* iter) {
	SordQuad tup;
	sord_iter_get(iter, tup);
	return tup[SORD_PREDICATE];
}

static inline const SordNode*
lilv_match_object(SordIter* iter) {
	SordQuad tup;
	sord_iter_get(iter, tup);
	return tup[SORD_OBJECT];
}

static inline const SordNode*
lilv_match_graph(SordIter* iter) {
	SordQuad tup;
	sord_iter_get(iter, tup);
	return tup[SORD_GRAPH];
}

static inline void
lilv_match_end(SordIter* iter)
{
	sord_iter_free(iter);
}

static inline bool lilv_matches_next(SordIter* matches) {
	return sord_iter_next(matches);
}

static inline bool lilv_matches_end(SordIter* matches) {
	return sord_iter_end(matches);
}

LilvNodes* lilv_nodes_from_stream_objects(LilvWorld* w,
                                          SordIter*  stream,
                                          bool       object);

char*  lilv_strjoin(const char* first, ...);
char*  lilv_strdup(const char* str);
char*  lilv_get_lang(void);
char*  lilv_expand(const char* path);
char*  lilv_dirname(const char* path);
int    lilv_copy_file(const char* src, const char* dst);
bool   lilv_path_exists(const char* path, void* ignored);
bool   lilv_path_is_absolute(const char* path);
char*  lilv_get_latest_copy(const char* path);
char*  lilv_path_relative_to(const char* path, const char* base);
bool   lilv_path_is_child(const char* path, const char* dir);
int    lilv_flock(FILE* file, bool lock);
char*  lilv_realpath(const char* path);
int    lilv_symlink(const char* oldpath, const char* newpath);
int    lilv_mkdir_p(const char* path);
char*  lilv_path_join(const char* a, const char* b);

char*
lilv_find_free_path(const char* in_path,
                    bool (*exists)(const char*, void*), void* user_data);

void
lilv_dir_for_each(const char* path,
                  void*       data,
                  void (*f)(const char* path, const char* name, void* data));

typedef void (*VoidFunc)(void);

/** dlsym wrapper to return a function pointer (without annoying warning) */
static inline VoidFunc
lilv_dlfunc(void* handle, const char* symbol)
{
	typedef VoidFunc (*VoidFuncGetter)(void*, const char*);
	VoidFuncGetter dlfunc = (VoidFuncGetter)dlsym;
	return dlfunc(handle, symbol);
}

#ifdef LILV_DYN_MANIFEST
static const LV2_Feature* const dman_features = { NULL };
#endif

#define LILV_ERROR(str)       fprintf(stderr, "%s(): error: " str, \
                                      __func__)
#define LILV_ERRORF(fmt, ...) fprintf(stderr, "%s(): error: " fmt, \
                                      __func__, __VA_ARGS__)
#define LILV_WARN(str)        fprintf(stderr, "%s(): warning: " str, \
                                      __func__)
#define LILV_WARNF(fmt, ...)  fprintf(stderr, "%s(): warning: " fmt, \
                                      __func__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* LILV_INTERNAL_H */
