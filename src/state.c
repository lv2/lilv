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

#define _BSD_SOURCE 1  /* for realpath, symlink */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "lilv-config.h"
#include "lilv_internal.h"

#ifdef HAVE_LV2_STATE
#    include "lv2/lv2plug.in/ns/ext/state/state.h"
#endif

#ifdef HAVE_MKDIR
#    include <sys/stat.h>
#    include <sys/types.h>
#endif

#include <unistd.h>

#define NS_ATOM  "http://lv2plug.in/ns/ext/atom#"
#define NS_PSET  "http://lv2plug.in/ns/ext/presets#"
#define NS_STATE "http://lv2plug.in/ns/ext/state#"

#define USTR(s) ((const uint8_t*)(s))

typedef struct {
	void*    value;
	size_t   size;
	uint32_t key;
	uint32_t type;
	uint32_t flags;
} Property;

typedef struct {
	char*     symbol;
	LilvNode* value;
} PortValue;

typedef struct {
	char* abs;  ///< Absolute path of actual file
	char* rel;  ///< Abstract path (relative path in state dir)
} PathMap;

struct LilvStateImpl {
	LilvNode*  plugin_uri;
	char*      dir;       ///< Save directory (if saved)
	char*      file_dir;  ///< Directory of files created by plugin
	char*      label;
	ZixTree*   abs2rel;  ///< PathMap sorted by abs
	ZixTree*   rel2abs;  ///< PathMap sorted by rel
	Property*  props;
	PortValue* values;
	uint32_t   state_Path;
	uint32_t   num_props;
	uint32_t   num_values;
};

static int
abs_cmp(const void* a, const void* b, void* user_data)
{
	return strcmp(((const PathMap*)a)->abs,
	              ((const PathMap*)b)->abs);
}

static int
rel_cmp(const void* a, const void* b, void* user_data)
{
	return strcmp(((const PathMap*)a)->rel,
	              ((const PathMap*)b)->rel);
}

static void
path_rel_free(void* ptr)
{
	free(((PathMap*)ptr)->abs);
	free(((PathMap*)ptr)->rel);
	free(ptr);
}

static int
property_cmp(const void* a, const void* b)
{
	const Property* pa = (const Property*)a;
	const Property* pb = (const Property*)b;
	return pa->key - pb->key;
}

static int
value_cmp(const void* a, const void* b)
{
	const PortValue* pa = (const PortValue*)a;
	const PortValue* pb = (const PortValue*)b;
	return strcmp(pa->symbol, pb->symbol);
}

static PortValue*
append_port_value(LilvState*  state,
                  const char* port_symbol,
                  LilvNode*   value)
{
	state->values = realloc(state->values,
	                        (++state->num_values) * sizeof(PortValue));
	PortValue* pv = &state->values[state->num_values - 1];
	pv->symbol = lilv_strdup(port_symbol);
	pv->value  = value;
	return pv;
}

#ifdef HAVE_LV2_STATE

static int
store_callback(void*       handle,
               uint32_t    key,
               const void* value,
               size_t      size,
               uint32_t    type,
               uint32_t    flags)
{
	LilvState* const state = (LilvState*)handle;
	state->props = realloc(state->props,
	                       (++state->num_props) * sizeof(Property));
	Property* const prop = &state->props[state->num_props - 1];

	if ((flags & LV2_STATE_IS_POD) || type == state->state_Path) {
		prop->value = malloc(size);
		memcpy(prop->value, value, size);
	} else {
		LILV_WARN("Storing non-POD value\n");
		prop->value = (void*)value;
	}

	prop->size  = size;
	prop->key   = key;
	prop->type  = type;
	prop->flags = flags;

	return 0;
}

static const void*
retrieve_callback(void*     handle,
                  uint32_t  key,
                  size_t*   size,
                  uint32_t* type,
                  uint32_t* flags)
{
	const LilvState* const state      = (LilvState*)handle;
	const Property         search_key = { NULL, 0, key, 0, 0 };
	const Property* const  prop       = (Property*)bsearch(
		&search_key, state->props, state->num_props,
		sizeof(Property), property_cmp);

	if (prop) {
		*size  = prop->size;
		*type  = prop->type;
		*flags = prop->flags;
		return prop->value;
	}
	return NULL;
}

static const char*
lilv_state_rel2abs(const LilvState* state, const char* path)
{
	ZixTreeIter*  iter = NULL;
	const PathMap key  = { NULL, (char*)path };
	if (state->rel2abs && !zix_tree_find(state->rel2abs, &key, &iter)) {
		return ((const PathMap*)zix_tree_get(iter))->abs;
	}
	return path;
}

static bool
lilv_state_has_path(const char* path, void* state)
{
	return lilv_state_rel2abs((LilvState*)state, path) != path;
}

static char*
abstract_path(LV2_State_Map_Path_Handle handle,
              const char*               absolute_path)
{
	LilvState*    state        = (LilvState*)handle;
	const size_t  file_dir_len = state->file_dir ? strlen(state->file_dir) : 0;
	char*         path         = NULL;
	char*         real_path    = realpath(absolute_path, NULL);
	const PathMap key          = { (char*)real_path, NULL };
	ZixTreeIter*  iter         = NULL;

	if (!zix_tree_find(state->abs2rel, &key, &iter)) {
		// Already mapped path in a previous call
		PathMap* pm = (PathMap*)zix_tree_get(iter);
		free(real_path);
		return lilv_strdup(pm->rel);
	} else if (lilv_path_is_child(real_path, state->file_dir)) {
		// File created by plugin
		char* copy = lilv_get_latest_copy(real_path);
		if (!copy) {
			// No recent enough copy, make a new one
			copy = lilv_find_free_path(real_path, lilv_path_exists, NULL);
			lilv_copy_file(real_path, copy);
		}
		real_path = copy;
			
		// Refer to the latest copy in plugin state
		path = lilv_strdup(copy + file_dir_len + 1);
	} else {
		// New path outside state directory
		const char* slash = strrchr(real_path, '/');
		const char* name  = slash ? (slash + 1) : real_path;

		// Find a free name in the (virtual) state directory
		path = lilv_find_free_path(name, lilv_state_has_path, state);
	}

	// Add record to path mapping
	PathMap* pm = malloc(sizeof(PathMap));
	pm->abs = real_path;
	pm->rel = lilv_strdup(path);
	zix_tree_insert(state->abs2rel, pm, NULL);
	zix_tree_insert(state->rel2abs, pm, NULL);

	return path;
}

static char*
absolute_path(LV2_State_Map_Path_Handle handle,
              const char*               abstract_path)
{
	LilvState* state = (LilvState*)handle;
	char*      path  = NULL;
	if (lilv_path_is_absolute(abstract_path)) {
		// Absolute path, return identical path
		path = lilv_strdup(abstract_path);
	} else {
		// Relative path inside state directory
		path = lilv_strjoin(state->dir, "/", abstract_path, NULL);
	}

	return path;
}

#endif  // HAVE_LV2_STATE

/** Return a new features array which is @c feature added to @c features. */
const LV2_Feature**
add_feature(const LV2_Feature *const * features, const LV2_Feature* feature)
{
	size_t n_features = 0;
	for (; features && features[n_features]; ++n_features) {}

	const LV2_Feature** ret = malloc((n_features + 2) * sizeof(LV2_Feature*));

	ret[0] = feature;
	if (features) {
		memcpy(ret + 1, features, n_features * sizeof(LV2_Feature*));
	}
	ret[n_features + 1] = NULL;
	return ret;
}

LILV_API
LilvState*
lilv_state_new_from_instance(const LilvPlugin*          plugin,
                             LilvInstance*              instance,
                             LV2_URID_Map*              map,
                             const char*                dir,
                             LilvGetPortValueFunc       get_value,
                             void*                      user_data,
                             uint32_t                   flags,
                             const LV2_Feature *const * features)
{
	const LV2_Feature** local_features = NULL;
	LilvWorld* const    world          = plugin->world;
	LilvState* const    state          = malloc(sizeof(LilvState));
	memset(state, '\0', sizeof(LilvState));
	state->plugin_uri = lilv_node_duplicate(lilv_plugin_get_uri(plugin));
	state->abs2rel    = zix_tree_new(false, abs_cmp, NULL, path_rel_free);
	state->rel2abs    = zix_tree_new(false, rel_cmp, NULL, NULL);
	state->file_dir   = dir ? realpath(dir, NULL) : NULL;
	state->state_Path = map->map(map->handle, LV2_STATE_PATH_URI);

#ifdef HAVE_LV2_STATE
	if (dir) {
		LV2_State_Map_Path map_path = { state, abstract_path, absolute_path };
		LV2_Feature        feature  = { LV2_STATE_MAP_PATH_URI, &map_path };
		features = local_features = add_feature(features, &feature);
	}
#endif

	// Store port values
	LilvNode* lv2_ControlPort = lilv_new_uri(world, LILV_URI_CONTROL_PORT);
	LilvNode* lv2_InputPort   = lilv_new_uri(world, LILV_URI_INPUT_PORT);
	for (uint32_t i = 0; i < plugin->num_ports; ++i) {
		const LilvPort* const port = plugin->ports[i];
		if (lilv_port_is_a(plugin, port, lv2_ControlPort)
		    && lilv_port_is_a(plugin, port, lv2_InputPort)) {
			const char* sym = lilv_node_as_string(port->symbol);
			append_port_value(state, sym, get_value(sym, user_data));
		}
	}
	lilv_node_free(lv2_ControlPort);
	lilv_node_free(lv2_InputPort);

	// Store properties
#ifdef HAVE_LV2_STATE
	const LV2_Descriptor*      descriptor = instance->lv2_descriptor;
	const LV2_State_Interface* iface      = (descriptor->extension_data)
		? descriptor->extension_data(LV2_STATE_INTERFACE_URI)
		: NULL;

	if (iface) {
		iface->save(instance->lv2_handle, store_callback, state, flags, features);
	}
#endif  // HAVE_LV2_STATE

	qsort(state->props, state->num_props, sizeof(Property), property_cmp);
	qsort(state->values, state->num_values, sizeof(PortValue), value_cmp);

	free(local_features);
	return state;
}

LILV_API
void
lilv_state_restore(const LilvState*           state,
                   LilvInstance*              instance,
                   LilvSetPortValueFunc       set_value,
                   void*                      user_data,
                   uint32_t                   flags,
                   const LV2_Feature *const * features)
{
#ifdef HAVE_LV2_STATE
	LV2_State_Map_Path map_path = { (LilvState*)state, abstract_path, absolute_path };
	LV2_Feature        feature  = { LV2_STATE_MAP_PATH_URI, &map_path };
	
	const LV2_Feature** local_features = add_feature(features, &feature);
	features = local_features;

	const LV2_Descriptor*      descriptor = instance->lv2_descriptor;
	const LV2_State_Interface* iface      = (descriptor->extension_data)
		? descriptor->extension_data(LV2_STATE_INTERFACE_URI)
		: NULL;

	if (iface) {
		iface->restore(instance->lv2_handle, retrieve_callback,
		               (LV2_State_Handle)state, flags, features);
	}

	free(local_features);

#endif  // HAVE_LV2_STATE

	if (set_value) {
		for (uint32_t i = 0; i < state->num_values; ++i) {
			set_value(state->values[i].symbol,
			          state->values[i].value,
			          user_data);
		}
	}
}

static SordNode*
get_one(SordModel* model, const SordNode* s, const SordNode* p)
{
	const SordQuad  pat  = { s, p, NULL, NULL };
	SordIter* const i    = sord_find(model, pat);
	SordNode* const node = i ? sord_node_copy(lilv_match_object(i)) : NULL;
	sord_iter_free(i);
	return node;
}

static void
property_from_node(LilvWorld*      world,
                   LV2_URID_Map*   map,
                   const LilvNode* node,
                   Property*       prop)
{
	const char* str = lilv_node_as_string(node);
	switch (node->type) {
	case LILV_VALUE_URI:
		prop->value = malloc(sizeof(uint32_t));
		*(uint32_t*)prop->value = map->map(map->handle, str);
		prop->type = map->map(map->handle, NS_ATOM "URID");
		prop->size = sizeof(uint32_t);
		break;
	case LILV_VALUE_STRING:
		prop->size = strlen(str) + 1;
		prop->value = malloc(prop->size);
		memcpy(prop->value, str, prop->size);
		prop->type = map->map(map->handle, NS_ATOM "String");
		break;
	case LILV_VALUE_BOOL:
		prop->value = malloc(sizeof(uint32_t));
		*(uint32_t*)prop->value = lilv_node_as_bool(node) ? 1 : 0;
		prop->type = map->map(map->handle, NS_ATOM "Bool");
		prop->size = sizeof(uint32_t);
		break;
	case LILV_VALUE_INT:
		prop->value = malloc(sizeof(uint32_t));
		*(uint32_t*)prop->value = lilv_node_as_int(node);
		prop->type = map->map(map->handle, NS_ATOM "Int32");
		prop->size = sizeof(uint32_t);
		break;
	case LILV_VALUE_FLOAT:
		prop->value = malloc(sizeof(float));
		*(float*)prop->value = lilv_node_as_float(node);
		prop->type = map->map(map->handle, NS_ATOM "Float");
		prop->size = sizeof(float);
		break;
	case LILV_VALUE_BLANK:
	case LILV_VALUE_BLOB:
		// TODO: Blank nodes in state
		break;
	}
}

static LilvState*
new_state_from_model(LilvWorld*       world,
                     LV2_URID_Map*    map,
                     SordModel*       model,
                     const SordNode*  node,
                     const char*      dir)
{
	LilvState* const state = malloc(sizeof(LilvState));
	memset(state, '\0', sizeof(LilvState));
	state->state_Path = map->map(map->handle, LV2_STATE_PATH_URI);
	state->dir        = dir ? lilv_strdup(dir) : NULL;

	// Get the plugin URI this state applies to
	const SordQuad upat = {
		node, world->uris.lv2_appliesTo, NULL, NULL };
	SordIter* i = sord_find(model, upat);
	if (i) {
		state->plugin_uri = lilv_node_new_from_node(
			world, lilv_match_object(i));
		if (!state->dir) {
			state->dir = lilv_strdup(
				(const char*)sord_node_get_string(lilv_match_graph(i)));
		}
		sord_iter_free(i);
	} else {
		LILV_ERRORF("State %s missing lv2:appliesTo property\n",
		            sord_node_get_string(node));
	}

	// Get the state label
	const SordQuad lpat = {
		node, world->uris.rdfs_label, NULL, NULL };
	i = sord_find(model, lpat);
	if (i) {
		state->label = lilv_strdup(
			(const char*)sord_node_get_string(lilv_match_object(i)));
		if (!state->dir) {
			state->dir = lilv_strdup(
				(const char*)sord_node_get_string(lilv_match_graph(i)));
		}
		sord_iter_free(i);
	}

	// Get port values
	const SordQuad ppat = { node, world->uris.lv2_port, NULL, NULL };
	SordIter* ports = sord_find(model, ppat);
	FOREACH_MATCH(ports) {
		const SordNode* port   = lilv_match_object(ports);
		const SordNode* label  = get_one(model, port, world->uris.rdfs_label);
		const SordNode* symbol = get_one(model, port, world->uris.lv2_symbol);
		const SordNode* value  = get_one(model, port, world->uris.pset_value);
		if (!symbol) {
			LILV_ERRORF("State `%s' port missing symbol.\n",
			            sord_node_get_string(node));
		} else if (!value) {
			LILV_ERRORF("State `%s' port `%s' missing value.\n",
			            sord_node_get_string(symbol),
			            sord_node_get_string(node));
		} else {
			const char* sym    = (const char*)sord_node_get_string(symbol);
			LilvNode*   lvalue = lilv_node_new_from_node(world, value);
			append_port_value(state, sym, lvalue);

			if (label) {
				lilv_state_set_label(
					state, (const char*)sord_node_get_string(label));
			}
		}
	}
	sord_iter_free(ports);

	SordNode* state_path_node = sord_new_uri(world->world,
	                                         USTR(LV2_STATE_PATH_URI));
	// Get properties
	SordNode* statep = sord_new_uri(world->world, USTR(NS_STATE "state"));
	const SordNode* state_node = get_one(model, node, statep);
	if (state) {
		const SordQuad spat  = { state_node, NULL, NULL };
		SordIter*      props = sord_find(model, spat);
		FOREACH_MATCH(props) {
			const SordNode* p = lilv_match_predicate(props);
			const SordNode* o = lilv_match_object(props);

			uint32_t flags = 0;
#ifdef HAVE_LV2_STATE
			flags = LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE;
#endif
			Property prop = { NULL, 0, 0, 0, flags };
			prop.key      = map->map(map->handle,
			                         (const char*)sord_node_get_string(p));

			if (sord_node_get_type(o) == SORD_BLANK) {
				const SordNode* type  = get_one(model, o, world->uris.rdf_a);
				const SordNode* value = get_one(model, o, world->uris.rdf_value);
				if (type && value) {
					size_t         len;
					const uint8_t* b64 = sord_node_get_string_counted(value, &len);
					prop.value = serd_base64_decode(b64, len, &prop.size);
					prop.type = map->map(map->handle,
					                     (const char*)sord_node_get_string(type));
				} else {
					LILV_ERRORF("Unable to parse blank node property <%p>\n",
					            sord_node_get_string(p));
				}
#ifdef HAVE_LV2_STATE
			} else if (sord_node_equals(sord_node_get_datatype(o),
			                            state_path_node)) {
				prop.size  = strlen((const char*)sord_node_get_string(o)) + 1;
				prop.type  = map->map(map->handle, LV2_STATE_PATH_URI);
				prop.flags = LV2_STATE_IS_PORTABLE;
				prop.value = lilv_strjoin(
					state->dir, "/", sord_node_get_string(o), NULL);
#endif
			} else {
				LilvNode* onode = lilv_node_new_from_node(world, o);
				property_from_node(world, map, onode, &prop);
				lilv_node_free(onode);
			}

			if (prop.value) {
				state->props = realloc(
					state->props, (++state->num_props) * sizeof(Property));
				state->props[state->num_props - 1] = prop;
			}

		}
		sord_iter_free(props);
	}
	sord_node_free(world->world, statep);
	sord_node_free(world->world, state_path_node);

	qsort(state->props, state->num_props, sizeof(Property), property_cmp);
	qsort(state->values, state->num_values, sizeof(PortValue), value_cmp);

	return state;
}

LILV_API
LilvState*
lilv_state_new_from_world(LilvWorld*      world,
                          LV2_URID_Map*   map,
                          const LilvNode* node)
{
	if (!lilv_node_is_uri(node) && !lilv_node_is_blank(node)) {
		LILV_ERRORF("Subject `%s' is not a URI or blank node.\n",
		            lilv_node_as_string(node));
		return NULL;
	}

	LilvState* state = new_state_from_model(
		world, map, world->model, node->val.uri_val, NULL);

	return state;
}

LILV_API
LilvState*
lilv_state_new_from_file(LilvWorld*      world,
                         LV2_URID_Map*   map,
                         const LilvNode* subject,
                         const char*     path)
{
	if (subject && !lilv_node_is_uri(subject)
	    && !lilv_node_is_blank(subject)) {
		LILV_ERRORF("Subject `%s' is not a URI or blank node.\n",
		            lilv_node_as_string(subject));
		return NULL;
	}

	uint8_t*    uri    = (uint8_t*)lilv_strjoin("file://", path, NULL);
	SerdNode    base   = serd_node_from_string(SERD_URI, uri);
	SerdEnv*    env    = serd_env_new(&base);
	SordModel*  model  = sord_new(world->world, SORD_SPO, false);
	SerdReader* reader = sord_new_reader(model, env, SERD_TURTLE, NULL);

	serd_reader_read_file(reader, uri);

	SordNode* subject_node = (subject)
		? subject->val.uri_val
		: sord_node_from_serd_node(world->world, env, &base, NULL, NULL);

	char* dirname   = lilv_dirname(path);
	char* real_path = realpath(dirname, NULL);
	LilvState* state = new_state_from_model(
		world, map, model, subject_node, real_path);
	free(dirname);
	free(real_path);

	serd_reader_free(reader);
	sord_free(model);
	serd_env_free(env);
	free(uri);
	return state;
}

static LilvNode*
node_from_property(LilvWorld* world, LV2_URID_Unmap* unmap,
                   const char* type, void* value, size_t size)
{
	if (!strcmp(type, NS_ATOM "String")) {
		return lilv_new_string(world, (const char*)value);
	} else if (!strcmp(type, NS_ATOM "URID")) {
		const char* str = unmap->unmap(unmap->handle, *(uint32_t*)value);
		return lilv_new_uri(world, str);
	} else if (!strcmp(type, NS_ATOM "Int32")) {
		if (size == sizeof(int32_t)) {
			return lilv_new_int(world, *(int32_t*)value);
		} else {
			LILV_WARNF("Int32 property <%s> has size %zu\n", type, size);
		}
	} else if (!strcmp(type, NS_ATOM "Float")) {
		if (size == sizeof(float)) {
			return lilv_new_float(world, *(float*)value);
		} else {
			LILV_WARNF("Float property <%s> has size %zu\n", type, size);
		}
	} else if (!strcmp(type, NS_ATOM "Bool")) {
		if (size == sizeof(int32_t)) {
			return lilv_new_bool(world, *(int32_t*)value);
		} else {
			LILV_WARNF("Bool property <%s> has size %zu\n", type, size);
		}
	}
	return NULL;
}

static void
node_to_serd(const LilvNode* node, SerdNode* value, SerdNode* type)
{
	const char* type_uri = NULL;
	switch (node->type) {
	case LILV_VALUE_URI:
		*value = serd_node_from_string(SERD_URI, USTR(node->str_val));
		break;
	case LILV_VALUE_BLANK:
		*value = serd_node_from_string(SERD_BLANK, USTR(node->str_val));
		break;
	default:
		*value = serd_node_from_string(SERD_LITERAL, USTR(node->str_val));
		switch (node->type) {
		case LILV_VALUE_BOOL:
			type_uri = LILV_NS_XSD "boolean";
			break;
		case LILV_VALUE_INT:
			type_uri = LILV_NS_XSD "integer";
			break;
		case LILV_VALUE_FLOAT:
			type_uri = LILV_NS_XSD "decimal";
			break;
		default:
			break;
		}
	}
	*type = (type_uri)
		? serd_node_from_string(SERD_URI, USTR(type_uri))
		: SERD_NODE_NULL;
}

static int
add_state_to_manifest(const LilvNode* plugin_uri,
                      const char*     manifest_path,
                      const char*     state_uri,
                      const char*     state_file_uri)
{
	FILE* fd = fopen((char*)manifest_path, "a");
	if (!fd) {
		LILV_ERRORF("Failed to open %s (%s)\n",
		            manifest_path, strerror(errno));
		return 4;
	}

	// Make path relative if it is in the same directory as manifest
	const char* last_slash = strrchr(state_file_uri, '/');
	if (last_slash) {
		const size_t len = last_slash - state_file_uri;
		if (!strncmp(manifest_path, state_file_uri, len)) {
			state_file_uri = last_slash + 1;
		}
	}

	SerdEnv* env = serd_env_new(NULL);
	serd_env_set_prefix_from_strings(env, USTR("lv2"),  USTR(LILV_NS_LV2));
	serd_env_set_prefix_from_strings(env, USTR("pset"), USTR(NS_PSET));
	serd_env_set_prefix_from_strings(env, USTR("rdf"),  USTR(LILV_NS_RDF));
	serd_env_set_prefix_from_strings(env, USTR("rdfs"), USTR(LILV_NS_RDFS));

	lilv_flock(fd, true);

	char* const manifest_uri = lilv_strjoin("file://", manifest_path, NULL);

	SerdURI base_uri;
	SerdNode base = serd_node_new_uri_from_string(
		(const uint8_t*)manifest_uri, NULL, &base_uri);

	SerdWriter* writer = serd_writer_new(
		SERD_TURTLE, SERD_STYLE_ABBREVIATED|SERD_STYLE_CURIED,
		env, &base_uri,
		serd_file_sink,
		fd);

	fseek(fd, 0, SEEK_END);
	if (ftell(fd) == 0) {
		serd_env_foreach(env, (SerdPrefixSink)serd_writer_set_prefix, writer);
	} else {
		fprintf(fd, "\n");
	}

	if (!state_uri) {
		state_uri = state_file_uri;
	}

	SerdNode s    = serd_node_from_string(SERD_URI, USTR(state_uri));
	SerdNode file = serd_node_from_string(SERD_URI, USTR(state_file_uri));

	// <state> a pset:Preset
	SerdNode p = serd_node_from_string(SERD_URI, USTR(LILV_NS_RDF "type"));
	SerdNode o = serd_node_from_string(SERD_CURIE, USTR("pset:Preset"));
	serd_writer_write_statement(writer, 0, NULL, &s, &p, &o, NULL, NULL);

	// <state> rdfs:seeAlso <file>
	p = serd_node_from_string(SERD_URI, USTR(LILV_NS_RDFS "seeAlso"));
	serd_writer_write_statement(writer, 0, NULL, &s, &p, &file, NULL, NULL);

	// <state> lv2:appliesTo <plugin>
	p = serd_node_from_string(SERD_URI, USTR(LILV_NS_LV2 "appliesTo"));
	o = serd_node_from_string(
		SERD_URI, USTR(lilv_node_as_string(plugin_uri)));
	serd_writer_write_statement(writer, 0, NULL, &s, &p, &o, NULL, NULL);

	serd_writer_free(writer);
	serd_node_free(&base);

	lilv_flock(fd, false);

	fclose(fd);
	free(manifest_uri);
	serd_env_free(env);

	return 0;
}

static char*
pathify(const char* in)
{
	const size_t in_len  = strlen(in);

	char* out = calloc(in_len + 1, 1);
	for (size_t i = 0; i < in_len; ++i) {
		char c = in[i];
		if (!((c >= 'a' && c <= 'z')
		      || (c >= 'A' && c <= 'Z')
		      || (c >= '0' && c <= '9'))) {
			c = '-';
		}
		out[i] = c;
	}
	return out;
}

static int
mkdir_p(const char* dir_path)
{
	char*        path     = lilv_strdup(dir_path);
	const size_t path_len = strlen(path);
	for (size_t i = 1; i <= path_len; ++i) {
		if (path[i] == LILV_DIR_SEP[0] || path[i] == '\0') {
			path[i] = '\0';
			if (mkdir(path, 0755) && errno != EEXIST) {
				LILV_ERRORF("Failed to create %s (%s)\n",
				            path, strerror(errno));
				free(path);
				return 1;
			}
			path[i] = LILV_DIR_SEP[0];
		}
	}

	free(path);
	return 0;
}

static char*
lilv_default_state_dir(LilvWorld* world)
{
#ifdef HAVE_MKDIR
	// Use environment variable or default value if it is unset
	char* state_bundle = getenv("LV2_STATE_BUNDLE");
	if (!state_bundle) {
		state_bundle = LILV_DEFAULT_STATE_BUNDLE;
	}

	// Expand any variables and create if necessary
	return lilv_expand(state_bundle);
#else
	LILV_ERROR("Save to default state path but mkdir is unavailable.\n");
	return NULL;
#endif
}

LILV_API
int
lilv_state_save(LilvWorld*                 world,
                LV2_URID_Unmap*            unmap,
                const LilvState*           state,
                const char*                uri,
                const char*                dir,
                const char*                filename,
                const LV2_Feature *const * features)
{
	char* default_dir      = NULL;
	char* default_filename = NULL;
	if (!dir) {
		dir = default_dir = lilv_default_state_dir(world);
	}
	if (mkdir_p(dir)) {
		free(default_dir);
		return 1;
	}

	if (!filename) {
		filename = default_filename = pathify(state->label);
	}

	char* const path = lilv_strjoin(dir, "/", filename, ".ttl", NULL);
	FILE* fd = fopen(path, "w");
	if (!fd) {
		LILV_ERRORF("Failed to open %s (%s)\n", path, strerror(errno));
		free(default_dir);
		free(default_filename);
		free(path);
		return 4;
	}

	// FIXME: make parameter non-const?
	((LilvState*)state)->dir = lilv_strdup(dir);

	char* const manifest = lilv_strjoin(dir, "/manifest.ttl", NULL);

	SerdEnv* env = serd_env_new(NULL);
	serd_env_set_prefix_from_strings(env, USTR("lv2"),   USTR(LILV_NS_LV2));
	serd_env_set_prefix_from_strings(env, USTR("pset"),  USTR(NS_PSET));
	serd_env_set_prefix_from_strings(env, USTR("rdf"),   USTR(LILV_NS_RDF));
	serd_env_set_prefix_from_strings(env, USTR("rdfs"),  USTR(LILV_NS_RDFS));
	serd_env_set_prefix_from_strings(env, USTR("state"), USTR(NS_STATE));

	SerdNode lv2_appliesTo = serd_node_from_string(
		SERD_CURIE, USTR("lv2:appliesTo"));

	const SerdNode* plugin_uri = sord_node_to_serd_node(
		state->plugin_uri->val.uri_val);

	SerdNode subject = serd_node_from_string(SERD_URI, USTR(uri ? uri : ""));

	SerdWriter* writer = serd_writer_new(
		SERD_TURTLE,
		SERD_STYLE_ABBREVIATED|SERD_STYLE_CURIED,
		env,
		&SERD_URI_NULL,
		serd_file_sink,
		fd);

	serd_env_foreach(env, (SerdPrefixSink)serd_writer_set_prefix, writer);

	// <subject> a pset:Preset
	SerdNode p = serd_node_from_string(SERD_URI, USTR(LILV_NS_RDF "type"));
	SerdNode o = serd_node_from_string(SERD_CURIE, USTR("pset:Preset"));
	serd_writer_write_statement(writer, 0, NULL,
	                            &subject, &p, &o, NULL, NULL);

	// <subject> lv2:appliesTo <http://example.org/plugin>
	serd_writer_write_statement(writer, 0, NULL,
	                            &subject,
	                            &lv2_appliesTo,
	                            plugin_uri, NULL, NULL);

	// <subject> rdfs:label label
	if (state->label) {
		p = serd_node_from_string(SERD_URI, USTR(LILV_NS_RDFS "label"));
		o = serd_node_from_string(SERD_LITERAL, USTR(state->label));
		serd_writer_write_statement(writer, 0,
		                            NULL, &subject, &p, &o, NULL, NULL);
	}

	// Save port values
	for (uint32_t i = 0; i < state->num_values; ++i) {
		PortValue* const value = &state->values[i];

		const SerdNode port = serd_node_from_string(
			SERD_BLANK, USTR(value->symbol));

		// <> lv2:port _:symbol
		p = serd_node_from_string(SERD_URI, USTR(LILV_NS_LV2 "port"));
		serd_writer_write_statement(writer, SERD_ANON_O_BEGIN,
		                            NULL, &subject, &p, &port, NULL, NULL);

		// _:symbol lv2:symbol "symbol"
		p = serd_node_from_string(SERD_URI, USTR(LILV_NS_LV2 "symbol"));
		o = serd_node_from_string(SERD_LITERAL, USTR(value->symbol));
		serd_writer_write_statement(writer, SERD_ANON_CONT,
		                            NULL, &port, &p, &o, NULL, NULL);

		// _:symbol pset:value value
		p = serd_node_from_string(SERD_URI, USTR(NS_PSET "value"));
		SerdNode t;
		node_to_serd(value->value, &o, &t);
		serd_writer_write_statement(writer, SERD_ANON_CONT, NULL,
		                            &port, &p, &o, &t, NULL);

		serd_writer_end_anon(writer, &port);
	}

	// Create symlinks to external files
#ifdef HAVE_LV2_STATE
	for (ZixTreeIter* i = zix_tree_begin(state->abs2rel);
	     i != zix_tree_end(state->abs2rel);
	     i = zix_tree_iter_next(i)) {
		const PathMap* pm = (const PathMap*)zix_tree_get(i);
		
		char* real_dir    = lilv_strjoin(realpath(dir, NULL), "/", NULL);
		char* rel_path    = lilv_strjoin(dir, "/", pm->rel, NULL);
		char* target_path = lilv_path_is_child(pm->abs, state->file_dir)
			? lilv_path_relative_to(pm->abs, real_dir)
			: lilv_strdup(pm->abs);
		if (symlink(target_path, rel_path)) {
			LILV_ERRORF("Failed to link `%s' => `%s' (%s)\n",
			            pm->abs, pm->rel, strerror(errno));
		}
		free(target_path);
		free(rel_path);
	}
#endif

	// Save properties
	const SerdNode state_node = serd_node_from_string(SERD_BLANK,
	                                                  USTR("2state"));
	if (state->num_props > 0) {
		p = serd_node_from_string(SERD_URI, USTR(NS_STATE "state"));
		serd_writer_write_statement(writer, SERD_ANON_O_BEGIN, NULL,
		                            &subject, &p, &state_node, NULL, NULL);

	}
	for (uint32_t i = 0; i < state->num_props; ++i) {
		Property*   prop = &state->props[i];
		const char* key  = unmap->unmap(unmap->handle, prop->key);
		const char* type = unmap->unmap(unmap->handle, prop->type);
		if (!key) {
			LILV_WARNF("Failed to unmap property key `%d'\n", prop->key);
		} else if (!type) {
			LILV_WARNF("Failed to unmap property type `%d'\n", prop->type);
		} else {
			SerdNode t;
			p = serd_node_from_string(SERD_URI, USTR(key));
			LilvNode* const node = node_from_property(
				world, unmap, type, prop->value, prop->size);
			if (node) {
				node_to_serd(node, &o, &t);
				// <state> <key> value
				serd_writer_write_statement(
					writer, SERD_ANON_CONT, NULL,
					&state_node, &p, &o, &t, NULL);
				lilv_node_free(node);
#ifdef HAVE_LV2_STATE
			} else if (!strcmp(type, NS_STATE "Path")) {
				o = serd_node_from_string(SERD_LITERAL, prop->value);
				t = serd_node_from_string(SERD_URI, (const uint8_t*)type);
				// <state> <key> "the/path"^^<state:Path>
				serd_writer_write_statement(
					writer, SERD_ANON_CONT, NULL,
					&state_node, &p, &o, &t, NULL);
#endif 
			} else {
				char name[16];
				snprintf(name, sizeof(name), "b%u", i);
				const SerdNode blank = serd_node_from_string(
					SERD_BLANK, (const uint8_t*)name);

				// <state> <key> [
				serd_writer_write_statement(
					writer, SERD_ANON_CONT|SERD_ANON_O_BEGIN, NULL,
					&state_node, &p, &blank, NULL, NULL);

				// rdf:type <type>
				p = serd_node_from_string(SERD_URI, USTR(LILV_NS_RDF "type"));
				o = serd_node_from_string(SERD_URI, USTR(type));
				serd_writer_write_statement(writer, SERD_ANON_CONT, NULL,
				                            &blank, &p, &o, NULL, NULL);

				// rdf:value "string"^^<xsd:base64Binary>
				SerdNode blob = serd_node_new_blob(prop->value, prop->size, true);
				p = serd_node_from_string(SERD_URI, USTR(LILV_NS_RDF "value"));
				t = serd_node_from_string(SERD_URI,
				                          USTR(LILV_NS_XSD "base64Binary"));
				serd_writer_write_statement(writer, SERD_ANON_CONT, NULL,
				                            &blank, &p, &blob, &t, NULL);
				serd_node_free(&blob);

				serd_writer_end_anon(writer, &blank);  // ]
			}
		}
	}
	if (state->num_props > 0) {
		serd_writer_end_anon(writer, &state_node);
	}

	// Close state file and clean up Serd
	serd_writer_free(writer);
	fclose(fd);
	serd_env_free(env);

	if (manifest) {
		add_state_to_manifest(state->plugin_uri, manifest, uri, path);
	}

	free(default_dir);
	free(default_filename);
	return 0;
}

LILV_API
void
lilv_state_free(LilvState* state)
{
	if (state) {
		for (uint32_t i = 0; i < state->num_props; ++i) {
			free(state->props[i].value);
		}
		for (uint32_t i = 0; i < state->num_values; ++i) {
			lilv_node_free(state->values[i].value);
			free(state->values[i].symbol);
		}
		lilv_node_free(state->plugin_uri);
		zix_tree_free(state->abs2rel);
		zix_tree_free(state->rel2abs);
		free(state->props);
		free(state->values);
		free(state->label);
		free(state->dir);
		free(state->file_dir);
		free(state);
	}
}

LILV_API
bool
lilv_state_equals(const LilvState* a, const LilvState* b)
{
	if (!lilv_node_equals(a->plugin_uri, b->plugin_uri)
	    || (a->label && !b->label)
	    || (b->label && !a->label)
	    || (a->label && b->label && strcmp(a->label, b->label))
	    || a->num_props != b->num_props
	    || a->num_values != b->num_values) {
		return false;
	}

	for (uint32_t i = 0; i < a->num_values; ++i) {
		PortValue* const av = &a->values[i];
		PortValue* const bv = &b->values[i];
		if (strcmp(av->symbol, bv->symbol)) {
			return false;
		} else if (!lilv_node_equals(av->value, bv->value)) {
			return false;
		}
	}

	for (uint32_t i = 0; i < a->num_props; ++i) {
		Property* const ap = &a->props[i];
		Property* const bp = &b->props[i];
		if (ap->key != bp->key
		    || ap->type != bp->type
		    || ap->flags != bp->flags) {
			return false;
		}

		if (ap->type == a->state_Path) {
			const char* const a_abs  = lilv_state_rel2abs(a, ap->value);
			const char* const b_abs  = lilv_state_rel2abs(b, bp->value);
			char* const       a_real = realpath(a_abs, NULL);
			char* const       b_real = realpath(b_abs, NULL);
			const int         cmp    = strcmp(a_real, b_real);
			free(a_real);
			free(b_real);
			if (cmp) {
				return false;
			}
		} else if (ap->size != bp->size
		           || memcmp(ap->value, bp->value, ap->size)) {
			return false;
		}
	}

	return true;
}

LILV_API
unsigned
lilv_state_get_num_properties(const LilvState* state)
{
	return state->num_props;
}

LILV_API
const LilvNode*
lilv_state_get_plugin_uri(const LilvState* state)
{
	return state->plugin_uri;
}

LILV_API
const char*
lilv_state_get_label(const LilvState* state)
{
	return state->label;
}

LILV_API
void
lilv_state_set_label(LilvState*  state,
                     const char* label)
{
	if (state->label) {
		free(state->label);
	}

	state->label = label ? lilv_strdup(label) : NULL;
}
