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

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lilv-config.h"
#include "lilv_internal.h"

#define NS_UI   (const uint8_t*)"http://lv2plug.in/ns/extensions/ui#"
#define NS_DOAP (const uint8_t*)"http://usefulinc.com/ns/doap#"
#define NS_FOAF (const uint8_t*)"http://xmlns.com/foaf/0.1/"

/** Ownership of @a uri is taken */
LilvPlugin
lilv_plugin_new(LilvWorld world, LilvValue uri, LilvValue bundle_uri)
{
	assert(bundle_uri);
	struct _LilvPlugin* plugin = malloc(sizeof(struct _LilvPlugin));
	plugin->world        = world;
	plugin->plugin_uri   = uri;
	plugin->bundle_uri   = bundle_uri;
	plugin->binary_uri   = NULL;
#ifdef LILV_DYN_MANIFEST
	plugin->dynman_uri   = NULL;
#endif
	plugin->plugin_class = NULL;
	plugin->data_uris    = lilv_values_new();
	plugin->ports        = NULL;
	plugin->num_ports    = 0;
	plugin->loaded       = false;
	plugin->replaced     = false;

	return plugin;
}

void
lilv_plugin_free(LilvPlugin p)
{
	lilv_value_free(p->plugin_uri);
	p->plugin_uri = NULL;

	lilv_value_free(p->bundle_uri);
	p->bundle_uri = NULL;

	lilv_value_free(p->binary_uri);
	p->binary_uri = NULL;

#ifdef LILV_DYN_MANIFEST
	lilv_value_free(p->dynman_uri);
	p->dynman_uri = NULL;
#endif

	if (p->ports) {
		for (uint32_t i = 0; i < p->num_ports; ++i)
			lilv_port_free(p->ports[i]);
		free(p->ports);
		p->ports = NULL;
	}

	lilv_values_free(p->data_uris);
	p->data_uris = NULL;

	free(p);
}

static LilvValues
lilv_plugin_query_node(LilvPlugin p, LilvNode subject, LilvNode predicate)
{
	lilv_plugin_load_if_necessary(p);
	// <subject> <predicate> ?value
	LilvMatches results = lilv_plugin_find_statements(
		p, subject, predicate, NULL);

	if (lilv_matches_end(results)) {
		lilv_match_end(results);
		return NULL;
	}

	LilvValues result = lilv_values_new();
	FOREACH_MATCH(results) {
		LilvNode  node  = lilv_match_object(results);
		LilvValue value = lilv_value_new_from_node(p->world, node);
		if (value)
			lilv_array_append(result, value);
	}
	lilv_match_end(results);

	return result;
}

LilvValue
lilv_plugin_get_unique(LilvPlugin p, LilvNode subject, LilvNode predicate)
{
	LilvValues values = lilv_plugin_query_node(p, subject, predicate);
	if (!values || lilv_values_size(values) != 1) {
		LILV_ERRORF("Port does not have exactly one `%s' property\n",
		            sord_node_get_string(predicate));
		return NULL;
	}
	LilvValue ret = lilv_value_duplicate(lilv_values_get_first(values));
	lilv_values_free(values);
	return ret;
}

static LilvValue
lilv_plugin_get_one(LilvPlugin p, LilvNode subject, LilvNode predicate)
{
	LilvValues values = lilv_plugin_query_node(p, subject, predicate);
	if (!values) {
		return NULL;
	}
	LilvValue ret = lilv_value_duplicate(lilv_values_get_first(values));
	lilv_values_free(values);
	return ret;
}

static void
lilv_plugin_load(LilvPlugin p)
{
	// Parse all the plugin's data files into RDF model
	LILV_FOREACH(values, i, p->data_uris) {
		LilvValue data_uri_val = lilv_values_get(p->data_uris, i);
		sord_read_file(p->world->model,
		               sord_node_get_string(data_uri_val->val.uri_val),
		               p->bundle_uri->val.uri_val,
		               lilv_world_blank_node_prefix(p->world));
	}

#ifdef LILV_DYN_MANIFEST
	typedef void* LV2_Dyn_Manifest_Handle;
	// Load and parse dynamic manifest data, if this is a library
	if (p->dynman_uri) {
		const char* lib_path = lilv_uri_to_path(lilv_value_as_string(p->dynman_uri));
		void* lib = dlopen(lib_path, RTLD_LAZY);
		if (!lib) {
			LILV_WARNF("Unable to open dynamic manifest %s\n",
			           lilv_value_as_string(p->dynman_uri));
			return;
		}

		typedef int (*OpenFunc)(LV2_Dyn_Manifest_Handle*, const LV2_Feature *const *);
		OpenFunc open_func = (OpenFunc)lilv_dlfunc(lib, "lv2_dyn_manifest_open");
		LV2_Dyn_Manifest_Handle handle = NULL;
		if (open_func)
			open_func(&handle, &dman_features);

		typedef int (*GetDataFunc)(LV2_Dyn_Manifest_Handle handle,
		                           FILE*                   fp,
		                           const char*             uri);
		GetDataFunc get_data_func = (GetDataFunc)lilv_dlfunc(
			lib, "lv2_dyn_manifest_get_data");
		if (get_data_func) {
			FILE* fd = tmpfile();
			get_data_func(handle, fd, lilv_value_as_string(p->plugin_uri));
			rewind(fd);
			sord_read_file_handle(p->world->model,
			                      fd,
			                      (const uint8_t*)lilv_value_as_uri(p->dynman_uri),
			                      p->bundle_uri->val.uri_val,
			                      lilv_world_blank_node_prefix(p->world));
			fclose(fd);
		}

		typedef int (*CloseFunc)(LV2_Dyn_Manifest_Handle);
		CloseFunc close_func = (CloseFunc)lilv_dlfunc(lib, "lv2_dyn_manifest_close");
		if (close_func)
			close_func(handle);
	}
#endif

	p->loaded = true;
}

static void
lilv_plugin_load_ports_if_necessary(LilvPlugin p)
{
	if (!p->loaded)
		lilv_plugin_load(p);

	if (!p->ports) {
		p->ports = malloc(sizeof(LilvPort*));
		p->ports[0] = NULL;

		LilvMatches ports = lilv_plugin_find_statements(
			p,
			p->plugin_uri->val.uri_val,
			p->world->lv2_port_node,
			NULL);

		FOREACH_MATCH(ports) {
			LilvValue index  = NULL;
			LilvNode  port   = lilv_match_object(ports);
			LilvValue symbol = lilv_plugin_get_unique(
				p, port, p->world->lv2_symbol_node);

			if (!lilv_value_is_string(symbol)) {
				LILV_ERROR("port has a non-string symbol\n");
				p->num_ports = 0;
				goto error;
			}

			index = lilv_plugin_get_unique(p, port, p->world->lv2_index_node);

			if (!lilv_value_is_int(index)) {
				LILV_ERROR("port has a non-integer index\n");
				p->num_ports = 0;
				goto error;
			}

			uint32_t this_index = lilv_value_as_int(index);
			LilvPort this_port  = NULL;
			if (p->num_ports > this_index) {
				this_port = p->ports[this_index];
			} else {
				p->ports = realloc(p->ports, (this_index + 1) * sizeof(LilvPort*));
				memset(p->ports + p->num_ports, '\0',
				       (this_index - p->num_ports) * sizeof(LilvPort));
				p->num_ports = this_index + 1;
			}

			// Havn't seen this port yet, add it to array
			if (!this_port) {
				this_port = lilv_port_new(p->world,
				                          this_index,
				                          lilv_value_as_string(symbol));
				p->ports[this_index] = this_port;
			}

			LilvMatches types = lilv_plugin_find_statements(
				p, port, p->world->rdf_a_node, NULL);
			FOREACH_MATCH(types) {
				LilvNode type = lilv_match_object(types);
				if (sord_node_get_type(type) == SORD_URI) {
					lilv_array_append(
						this_port->classes,
						lilv_value_new_from_node(p->world, type));
				} else {
					LILV_WARN("port has non-URI rdf:type\n");
				}
			}
			lilv_match_end(types);

		error:
			lilv_value_free(symbol);
			lilv_value_free(index);
			if (p->num_ports == 0) {
				if (p->ports) {
					for (uint32_t i = 0; i < p->num_ports; ++i)
						lilv_port_free(p->ports[i]);
					free(p->ports);
					p->ports = NULL;
				}
				break; // Invalid plugin
			}
		}
		lilv_match_end(ports);
	}
}

void
lilv_plugin_load_if_necessary(LilvPlugin p)
{
	if (!p->loaded)
		lilv_plugin_load(p);
}

LILV_API
LilvValue
lilv_plugin_get_uri(LilvPlugin p)
{
	assert(p);
	assert(p->plugin_uri);
	return p->plugin_uri;
}

LILV_API
LilvValue
lilv_plugin_get_bundle_uri(LilvPlugin p)
{
	assert(p);
	assert(p->bundle_uri);
	return p->bundle_uri;
}

LILV_API
LilvValue
lilv_plugin_get_library_uri(LilvPlugin p)
{
	lilv_plugin_load_if_necessary(p);
	if (!p->binary_uri) {
		// <plugin> lv2:binary ?binary
		LilvMatches results = lilv_plugin_find_statements(
			p,
			p->plugin_uri->val.uri_val,
			p->world->lv2_binary_node,
			NULL);
		FOREACH_MATCH(results) {
			LilvNode binary_node = lilv_match_object(results);
			if (sord_node_get_type(binary_node) == SORD_URI) {
				p->binary_uri = lilv_value_new_from_node(p->world, binary_node);
				break;
			}
		}
		lilv_match_end(results);
	}
	if (!p->binary_uri) {
		LILV_WARNF("Plugin <%s> has no lv2:binary\n",
		           lilv_value_as_uri(lilv_plugin_get_uri(p)));
	}
	return p->binary_uri;
}

LILV_API
LilvValues
lilv_plugin_get_data_uris(LilvPlugin p)
{
	return p->data_uris;
}

LILV_API
LilvPluginClass
lilv_plugin_get_class(LilvPlugin p)
{
	lilv_plugin_load_if_necessary(p);
	if (!p->plugin_class) {
		// <plugin> a ?class
		LilvMatches results = lilv_plugin_find_statements(
			p,
			p->plugin_uri->val.uri_val,
			p->world->rdf_a_node,
			NULL);
		FOREACH_MATCH(results) {
			LilvNode class_node = lilv_match_object(results);
			if (sord_node_get_type(class_node) != SORD_URI) {
				continue;
			}

			LilvValue class = lilv_value_new_from_node(p->world, class_node);
			if ( ! lilv_value_equals(class, p->world->lv2_plugin_class->uri)) {

				LilvPluginClass plugin_class = lilv_plugin_classes_get_by_uri(
						p->world->plugin_classes, class);

				if (plugin_class) {
					p->plugin_class = plugin_class;
					lilv_value_free(class);
					break;
				}
			}

			lilv_value_free(class);
		}
		lilv_match_end(results);

		if (p->plugin_class == NULL)
			p->plugin_class = p->world->lv2_plugin_class;
	}
	return p->plugin_class;
}

LILV_API
bool
lilv_plugin_verify(LilvPlugin plugin)
{
	LilvValues results = lilv_plugin_get_value_by_qname(plugin, "rdf:type");
	if (!results) {
		return false;
	}

	lilv_values_free(results);
	results = lilv_plugin_get_value(plugin, plugin->world->doap_name_val);
	if (!results) {
		return false;
	}

	lilv_values_free(results);
	results = lilv_plugin_get_value_by_qname(plugin, "doap:license");
	if (!results) {
		return false;
	}

	lilv_values_free(results);
	results = lilv_plugin_get_value_by_qname(plugin, "lv2:port");
	if (!results) {
		return false;
	}

	lilv_values_free(results);
	return true;
}

LILV_API
LilvValue
lilv_plugin_get_name(LilvPlugin plugin)
{
	LilvValues results = lilv_plugin_get_value(plugin,
	                                           plugin->world->doap_name_val);

	LilvValue ret = NULL;
	if (results) {
		LilvValue val = lilv_values_get_first(results);
		if (lilv_value_is_string(val))
			ret = lilv_value_duplicate(val);
		lilv_values_free(results);
	}

	if (!ret)
		LILV_WARNF("<%s> has no (mandatory) doap:name\n",
		           lilv_value_as_string(lilv_plugin_get_uri(plugin)));

	return ret;
}

LILV_API
LilvValues
lilv_plugin_get_value(LilvPlugin p,
                      LilvValue  predicate)
{
	return lilv_plugin_get_value_for_subject(p, p->plugin_uri, predicate);
}

LILV_API
LilvValues
lilv_plugin_get_value_by_qname(LilvPlugin  p,
                               const char* predicate)
{
	char* pred_uri = (char*)lilv_qname_expand(p, predicate);
	if (!pred_uri) {
		return NULL;
	}
	LilvValue  pred_value = lilv_value_new_uri(p->world, pred_uri);
	LilvValues ret        = lilv_plugin_get_value(p, pred_value);

	lilv_value_free(pred_value);
	free(pred_uri);
	return ret;
}

LILV_API
LilvValues
lilv_plugin_get_value_for_subject(LilvPlugin p,
                                  LilvValue  subject,
                                  LilvValue  predicate)
{
	if ( ! lilv_value_is_uri(subject) && ! lilv_value_is_blank(subject)) {
		LILV_ERROR("Subject is not a resource\n");
		return NULL;
	}
	if ( ! lilv_value_is_uri(predicate)) {
		LILV_ERROR("Predicate is not a URI\n");
		return NULL;
	}

	SordNode* subject_node = (lilv_value_is_uri(subject))
		? lilv_node_copy(subject->val.uri_val)
		: sord_new_blank(p->world->world,
		                 (const uint8_t*)lilv_value_as_blank(subject));

	LilvValues ret = lilv_plugin_query_node(p,
	                                        subject_node,
	                                        predicate->val.uri_val);

	lilv_node_free(p->world, subject_node);
	return ret;
}

LILV_API
uint32_t
lilv_plugin_get_num_ports(LilvPlugin p)
{
	lilv_plugin_load_ports_if_necessary(p);
	return p->num_ports;
}

LILV_API
void
lilv_plugin_get_port_ranges_float(LilvPlugin p,
                                  float*     min_values,
                                  float*     max_values,
                                  float*     def_values)
{
	lilv_plugin_load_ports_if_necessary(p);
	for (uint32_t i = 0; i < p->num_ports; ++i) {
		LilvValue def, min, max;
		lilv_port_get_range(p, p->ports[i], &def, &min, &max);

		if (min_values)
			min_values[i] = min ? lilv_value_as_float(min) : NAN;

		if (max_values)
			max_values[i] = max ? lilv_value_as_float(max) : NAN;

		if (def_values)
			def_values[i] = def ? lilv_value_as_float(def) : NAN;

		lilv_value_free(def);
		lilv_value_free(min);
		lilv_value_free(max);
	}
}

LILV_API
uint32_t
lilv_plugin_get_num_ports_of_class(LilvPlugin p,
                                   LilvValue  class_1, ...)
{
	lilv_plugin_load_ports_if_necessary(p);

	uint32_t ret = 0;
	va_list  args;

	for (unsigned i = 0; i < p->num_ports; ++i) {
		LilvPort port = p->ports[i];
		if (!port || !lilv_port_is_a(p, port, class_1))
			continue;

		va_start(args, class_1);

		bool matches = true;
		for (LilvValue class_i = NULL; (class_i = va_arg(args, LilvValue)) != NULL ; ) {
			if (!lilv_port_is_a(p, port, class_i)) {
				va_end(args);
				matches = false;
				break;
			}
		}

		if (matches)
			++ret;

		va_end(args);
	}

	return ret;
}

LILV_API
bool
lilv_plugin_has_latency(LilvPlugin p)
{
	LilvMatches ports = lilv_plugin_find_statements(
		p,
		p->plugin_uri->val.uri_val,
		p->world->lv2_port_node,
		NULL);

	bool ret = false;
	FOREACH_MATCH(ports) {
		LilvNode    port            = lilv_match_object(ports);
		LilvMatches reports_latency = lilv_plugin_find_statements(
			p,
			port,
			p->world->lv2_portproperty_node,
			p->world->lv2_reportslatency_node);
		const bool end = lilv_matches_end(reports_latency);
		lilv_match_end(reports_latency);
		if (!end) {
			ret = true;
			break;
		}
	}
	lilv_match_end(ports);

	return ret;
}

LILV_API
uint32_t
lilv_plugin_get_latency_port_index(LilvPlugin p)
{
	LilvMatches ports = lilv_plugin_find_statements(
		p,
		p->plugin_uri->val.uri_val,
		p->world->lv2_port_node,
		NULL);

	uint32_t ret = 0;
	FOREACH_MATCH(ports) {
		LilvNode    port            = lilv_match_object(ports);
		LilvMatches reports_latency = lilv_plugin_find_statements(
			p,
			port,
			p->world->lv2_portproperty_node,
			p->world->lv2_reportslatency_node);
		if (!lilv_matches_end(reports_latency)) {
			LilvValue index = lilv_plugin_get_unique(
				p, port, p->world->lv2_index_node);

			ret = lilv_value_as_int(index);
			lilv_value_free(index);
			lilv_match_end(reports_latency);
			break;
		}
		lilv_match_end(reports_latency);
	}
	lilv_match_end(ports);

	return ret;  // FIXME: error handling
}

LILV_API
bool
lilv_plugin_has_feature(LilvPlugin p,
                        LilvValue  feature)
{
	LilvValues features = lilv_plugin_get_supported_features(p);

	const bool ret = features && feature && lilv_values_contains(features, feature);

	lilv_values_free(features);
	return ret;
}

LILV_API
LilvValues
lilv_plugin_get_supported_features(LilvPlugin p)
{
	LilvValues optional = lilv_plugin_get_optional_features(p);
	LilvValues required = lilv_plugin_get_required_features(p);
	LilvValues result   = lilv_values_new();

	LILV_FOREACH(values, i, optional)
		lilv_array_append(
			result, lilv_value_duplicate(lilv_values_get(optional, i)));
	LILV_FOREACH(values, i, required)
		lilv_array_append(
			result, lilv_value_duplicate(lilv_values_get(required, i)));

	lilv_values_free(optional);
	lilv_values_free(required);

	return result;
}

LILV_API
LilvValues
lilv_plugin_get_optional_features(LilvPlugin p)
{
	return lilv_plugin_get_value_by_qname(p, "lv2:optionalFeature");
}

LILV_API
LilvValues
lilv_plugin_get_required_features(LilvPlugin p)
{
	return lilv_plugin_get_value_by_qname(p, "lv2:requiredFeature");
}

LILV_API
LilvPort
lilv_plugin_get_port_by_index(LilvPlugin p,
                              uint32_t   index)
{
	lilv_plugin_load_ports_if_necessary(p);
	if (index < p->num_ports)
		return p->ports[index];
	else
		return NULL;
}

LILV_API
LilvPort
lilv_plugin_get_port_by_symbol(LilvPlugin p,
                               LilvValue  symbol)
{
	lilv_plugin_load_ports_if_necessary(p);
	for (uint32_t i = 0; i < p->num_ports; ++i) {
		LilvPort port = p->ports[i];
		if (lilv_value_equals(port->symbol, symbol))
			return port;
	}

	return NULL;
}

static LilvNode
lilv_plugin_get_author(LilvPlugin p)
{
	SordNode* doap_maintainer = sord_new_uri(
		p->world->world, NS_DOAP "maintainer");

	LilvMatches maintainers = lilv_plugin_find_statements(
		p,
		p->plugin_uri->val.uri_val,
		doap_maintainer,
		NULL);

	lilv_node_free(p->world, doap_maintainer);

	if (lilv_matches_end(maintainers)) {
		return NULL;
	}

	LilvNode author = lilv_match_object(maintainers);

	lilv_match_end(maintainers);
	return author;
}

LILV_API
LilvValue
lilv_plugin_get_author_name(LilvPlugin plugin)
{
	LilvNode author = lilv_plugin_get_author(plugin);
	if (author) {
		return lilv_plugin_get_one(
			plugin, author, sord_new_uri(
				plugin->world->world, NS_FOAF "name"));
	}
	return NULL;
}

LILV_API
LilvValue
lilv_plugin_get_author_email(LilvPlugin plugin)
{
	LilvNode author = lilv_plugin_get_author(plugin);
	if (author) {
		return lilv_plugin_get_one(
			plugin, author, sord_new_uri(
				plugin->world->world, NS_FOAF "mbox"));
	}
	return NULL;
}

LILV_API
LilvValue
lilv_plugin_get_author_homepage(LilvPlugin plugin)
{
	LilvNode author = lilv_plugin_get_author(plugin);
	if (author) {
		return lilv_plugin_get_one(
			plugin, author, sord_new_uri(
				plugin->world->world, NS_FOAF "homepage"));
	}
	return NULL;
}

LILV_API
bool
lilv_plugin_is_replaced(LilvPlugin plugin)
{
	return plugin->replaced;
}

LILV_API
LilvUIs
lilv_plugin_get_uis(LilvPlugin p)
{
	SordNode* ui_ui_node     = sord_new_uri(p->world->world, NS_UI "ui");
	SordNode* ui_binary_node = sord_new_uri(p->world->world, NS_UI "binary");

	LilvUIs     result = lilv_uis_new();
	LilvMatches uis    = lilv_plugin_find_statements(
		p,
		p->plugin_uri->val.uri_val,
		ui_ui_node,
		NULL);

	FOREACH_MATCH(uis) {
		LilvNode  ui     = lilv_match_object(uis);
		LilvValue type   = lilv_plugin_get_unique(p, ui, p->world->rdf_a_node);
		LilvValue binary = lilv_plugin_get_unique(p, ui, ui_binary_node);

		if (sord_node_get_type(ui) != SORD_URI
		    || !lilv_value_is_uri(type)
		    || !lilv_value_is_uri(binary)) {
			lilv_value_free(binary);
			lilv_value_free(type);
			LILV_ERROR("Corrupt UI\n");
			continue;
		}

		LilvUI lilv_ui = lilv_ui_new(
			p->world,
			lilv_value_new_from_node(p->world, ui),
			type,
			binary);

		lilv_sequence_insert(result, lilv_ui);
	}
	lilv_match_end(uis);

	lilv_node_free(p->world, ui_binary_node);
	lilv_node_free(p->world, ui_ui_node);

	if (lilv_uis_size(result) > 0) {
		return result;
	} else {
		lilv_uis_free(result);
		return NULL;
	}
}
