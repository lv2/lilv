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

#define _POSIX_SOURCE 1  /* for fileno */
#define _BSD_SOURCE   1  /* for lockf */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "lilv-config.h"
#include "lilv_internal.h"

#ifdef HAVE_LV2_STATE
#    include "lv2/lv2plug.in/ns/ext/state/state.h"
#endif

#if defined(HAVE_LOCKF) && defined(HAVE_FILENO)
#    include <unistd.h>
#endif

#ifdef HAVE_MKDIR
#    include <sys/stat.h>
#    include <sys/types.h>
#endif

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

struct LilvStateImpl {
	LilvNode*  plugin_uri;
	Property*  props;
	PortValue* values;
	char*      label;
	uint32_t   num_props;
	uint32_t   num_values;
};

static int
property_cmp(const void* a, const void* b)
{
	const Property* pa = (const Property*)a;
	const Property* pb = (const Property*)b;
	return pa->key - pb->key;
}

LILV_API
const LilvNode*
lilv_state_get_plugin_uri(const LilvState* state)
{
	return state->plugin_uri;
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

#ifdef HAVE_LV2_STATE
static int
store_callback(void*       handle,
               uint32_t    key,
               const void* value,
               size_t      size,
               uint32_t    type,
               uint32_t    flags)
{
	if (!(flags & LV2_STATE_IS_POD)) {
		// TODO: A flag so we know if we can hold a reference would be nice
		LILV_WARN("Non-POD property ignored.\n");
		return 1;
	}

	LilvState* const state = (LilvState*)handle;
	state->props = realloc(state->props,
	                       (++state->num_props) * sizeof(Property));
	Property* const prop = &state->props[state->num_props - 1];
	
	prop->value = malloc(size);
	memcpy(prop->value, value, size);

	prop->size  = size;
	prop->key   = key;
	prop->type  = type;
	prop->flags = flags;

	return 0;
}
#endif  // HAVE_LV2_STATE

LILV_API
LilvState*
lilv_state_new_from_instance(const LilvPlugin*          plugin,
                             LilvInstance*              instance,
                             LilvGetPortValueFunc       get_value,
                             void*                      user_data,
                             uint32_t                   flags,
                             const LV2_Feature *const * features)
{
	LilvWorld* const world = plugin->world;
	LilvState* const state = malloc(sizeof(LilvState));
	memset(state, '\0', sizeof(LilvState));
	state->plugin_uri = lilv_node_duplicate(lilv_plugin_get_uri(plugin));

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

	iface->save(instance->lv2_handle, store_callback, state, flags, features);
#endif  // HAVE_LV2_STATE

	qsort(state->props, state->num_props, sizeof(Property), property_cmp);

	return state;
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
	const LV2_Descriptor*      descriptor = instance->lv2_descriptor;
	const LV2_State_Interface* iface      = (descriptor->extension_data)
		? descriptor->extension_data(LV2_STATE_INTERFACE_URI)
		: NULL;

	iface->restore(instance->lv2_handle, retrieve_callback,
	               (LV2_State_Handle)state, flags, features);

	if (set_value) {
		for (uint32_t i = 0; i < state->num_values; ++i) {
			set_value(state->values[i].symbol,
			          state->values[i].value,
			          user_data);
		}
	}
#endif  // HAVE_LV2_STATE
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
	case LILV_VALUE_BLANK:
		// TODO: Hmm...
		break;
	case LILV_VALUE_STRING:
		prop->size = strlen(str) + 1;
		prop->value = malloc(prop->size);
		memcpy(prop->value, str, prop->size + 1);
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
	}
	prop->flags = LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE;
}

static LilvState*
new_state_from_model(LilvWorld*      world,
                     LV2_URID_Map*   map,
                     SordModel*      model,
                     const SordNode* node)
{
	LilvState* const state = malloc(sizeof(LilvState));
	memset(state, '\0', sizeof(LilvState));

	// Get the plugin URI this state applies to
	const SordQuad pat1 = {
		node, world->lv2_appliesTo_node, NULL, NULL };
	SordIter* i = sord_find(model, pat1);
	if (i) {
		state->plugin_uri = lilv_node_new_from_node(
			world, lilv_match_object(i));
		sord_iter_free(i);
	} else {
		LILV_ERRORF("State %s missing lv2:appliesTo property\n",
		            sord_node_get_string(node));
	}

	// Get port values
	const SordQuad pat2 = { node, world->lv2_port_node, NULL, NULL };
	SordIter* ports = sord_find(model, pat2);
	FOREACH_MATCH(ports) {
		const SordNode* port   = lilv_match_object(ports);
		const SordNode* label  = get_one(model, port, world->rdfs_label_node);
		const SordNode* symbol = get_one(model, port, world->lv2_symbol_node);
		const SordNode* value  = get_one(model, port, world->pset_value_node);
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

	// Get properties
	SordNode* statep = sord_new_uri(world->world, USTR(NS_STATE "state"));
	const SordNode* state_node = get_one(model, node, statep);
	if (state_node) {
		const SordQuad pat3  = { state_node, NULL, NULL };
		SordIter*      props = sord_find(model, pat3);
		FOREACH_MATCH(props) {
			const SordNode* p     = lilv_match_predicate(props);
			const SordNode* o     = lilv_match_object(props);
			LilvNode*       onode = lilv_node_new_from_node(world, o);
			
			Property prop = { NULL, 0, 0, 0, 0 };
			prop.key      = map->map(map->handle,
			                         (const char*)sord_node_get_string(p));
			property_from_node(world, map, onode, &prop);
			if (prop.value) {
				state->props = realloc(
					state->props, (++state->num_props) * sizeof(Property));
				state->props[state->num_props - 1] = prop;
			}

			lilv_node_free(onode);
		}
		sord_iter_free(props);
	}
	sord_node_free(world->world, statep);

	qsort(state->props, state->num_props, sizeof(Property), property_cmp);
	
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

	return new_state_from_model(world, map, world->model, node->val.uri_val);
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

	LilvState* state = new_state_from_model(world, map, model, subject_node);

	serd_reader_free(reader);
	sord_free(model);
	serd_env_free(env);
	free(uri);
	return state;
}

static LilvNode*
node_from_property(LilvWorld* world, const char* type, void* value, size_t size)
{
	if (!strcmp(type, NS_ATOM "String")) {
		return lilv_new_string(world, (const char*)value);
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
		fprintf(stderr, "error: Failed to open %s (%s)\n",
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
	serd_env_set_prefix_from_strings(env, USTR("rdfs"), USTR(LILV_NS_RDFS));

#if defined(HAVE_LOCKF) && defined(HAVE_FILENO)
	lockf(fileno(fd), F_LOCK, 0);
#endif

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

#ifdef HAVE_LOCKF
	lockf(fileno(fd), F_ULOCK, 0);
#endif

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

LILV_API
int
lilv_state_save(LilvWorld*       world,
                LV2_URID_Unmap*  unmap,
                const LilvState* state,
                const char*      uri,
                const char*      path,
                const char*      manifest_path)
{
	char* default_path          = NULL;
	char* default_manifest_path = NULL;
	if (!path) {
#ifdef HAVE_MKDIR
		if (!state->label) {
			LILV_ERROR("Attempt to save state with no label or path.\n");
			return 1;
		}

		const char* const home = getenv("HOME");
		if (!home) {
			fprintf(stderr, "error: $HOME is undefined\n");
			return 2;
		}

		// Create ~/.lv2/
		char* const lv2dir = lilv_strjoin(home, "/.lv2/", NULL);
		if (mkdir(lv2dir, 0755) && errno != EEXIST) {
			fprintf(stderr, "error: Unable to create %s (%s)\n",
			        lv2dir, strerror(errno));
			free(lv2dir);
			return 3;
		}

		// Create ~/.lv2/presets.lv2/
		char* const bundle = lilv_strjoin(lv2dir, "presets.lv2/", NULL);
		if (mkdir(bundle, 0755) && errno != EEXIST) {
			fprintf(stderr, "error: Unable to create %s (%s)\n",
			        lv2dir, strerror(errno));
			free(lv2dir);
			free(bundle);
			return 4;
		}
		
		char* const filename  = pathify(state->label);
		default_path          = lilv_strjoin(bundle, filename, ".ttl", NULL);
		default_manifest_path = lilv_strjoin(bundle, "manifest.ttl", NULL);

		path          = default_path;
		manifest_path = default_manifest_path;

		free(lv2dir);
		free(bundle);
		free(filename);
#else
		LILV_ERROR("Save to default state path but mkdir is unavailable.\n");
		return 1;
#endif
	}

	FILE* fd = fopen(path, "w");
	if (!fd) {
		fprintf(stderr, "error: Failed to open %s (%s)\n",
		        path, strerror(errno));
		free(default_path);
		free(default_manifest_path);
		return 4;
	}

	SerdEnv* env = serd_env_new(NULL);
	serd_env_set_prefix_from_strings(env, USTR("lv2"),   USTR(LILV_NS_LV2));
	serd_env_set_prefix_from_strings(env, USTR("pset"),  USTR(NS_PSET));
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
		} else if (!(prop->flags & LV2_STATE_IS_PORTABLE)) {
			LILV_WARNF("Unable to save non-portable property <%s>\n", type);
		} else {
			LilvNode* const node = node_from_property(
				world, type, prop->value, prop->size);
			if (node) {
				p = serd_node_from_string(SERD_URI, USTR(key));
				SerdNode t;
				node_to_serd(node, &o, &t);
				serd_writer_write_statement(
					writer, SERD_ANON_CONT, NULL,
					&state_node, &p, &o, &t, NULL);
			} else {
				LILV_WARNF("Unable to save property type <%s>\n", type);
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

	if (manifest_path) {
		add_state_to_manifest(
			state->plugin_uri, manifest_path, uri, path);
	}

	free(default_path);
	free(default_manifest_path);
	return 0;
}

LILV_API
void
lilv_state_free(LilvState* state)
{
	if (state) {
		lilv_node_free(state->plugin_uri);
		free(state->props);
		free(state->values);
		free(state->label);
		free(state);
	}
}
