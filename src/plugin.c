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

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef SLV2_DYN_MANIFEST
#include <dlfcn.h>
#endif
#include "slv2/types.h"
#include "slv2/collections.h"
#include "slv2/plugin.h"
#include "slv2/pluginclass.h"
#include "slv2/util.h"
#include "slv2_internal.h"

/* private
 * ownership of uri is taken */
SLV2Plugin
slv2_plugin_new(SLV2World world, SLV2Value uri, SLV2Value bundle_uri)
{
	assert(bundle_uri);
	struct _SLV2Plugin* plugin = malloc(sizeof(struct _SLV2Plugin));
	plugin->world        = world;
	plugin->plugin_uri   = uri;
	plugin->bundle_uri   = bundle_uri;
	plugin->binary_uri   = NULL;
#ifdef SLV2_DYN_MANIFEST
	plugin->dynman_uri   = NULL;
#endif
	plugin->plugin_class = NULL;
	plugin->data_uris    = slv2_values_new();
	plugin->ports        = NULL;
	plugin->num_ports    = 0;
	plugin->loaded       = false;

	return plugin;
}

/* private */
void
slv2_plugin_free(SLV2Plugin p)
{
	slv2_value_free(p->plugin_uri);
	p->plugin_uri = NULL;

	slv2_value_free(p->bundle_uri);
	p->bundle_uri = NULL;

	slv2_value_free(p->binary_uri);
	p->binary_uri = NULL;

#ifdef SLV2_DYN_MANIFEST
	slv2_value_free(p->dynman_uri);
	p->dynman_uri = NULL;
#endif

	if (p->ports) {
		for (uint32_t i = 0; i < p->num_ports; ++i)
			slv2_port_free(p->ports[i]);
		free(p->ports);
		p->ports = NULL;
	}

	slv2_values_free(p->data_uris);
	p->data_uris = NULL;

	free(p);
}

/* private */
void
slv2_plugin_load_if_necessary(SLV2Plugin p)
{
	if (!p->loaded)
		slv2_plugin_load(p);
}

static SLV2Values
slv2_plugin_query_node(SLV2Plugin p, SLV2Node subject, SLV2Node predicate)
{
	slv2_plugin_load_if_necessary(p);
	// <subject> <predicate> ?value
	SLV2Matches results = slv2_plugin_find_statements(
		p, subject, predicate, NULL);

	if (slv2_matches_end(results)) {
		slv2_match_end(results);
		return NULL;
	}

	SLV2Values result = slv2_values_new();
	FOREACH_MATCH(results) {
		SLV2Node  node  = slv2_match_object(results);
		SLV2Value value = slv2_value_new_from_node(p->world, node);
		if (value)
			g_ptr_array_add(result, value);
	}
	slv2_match_end(results);

	return result;
}

SLV2Value
slv2_plugin_get_unique(SLV2Plugin p, SLV2Node subject, SLV2Node predicate)
{
	SLV2Values values = slv2_plugin_query_node(p, subject, predicate);
	if (!values || slv2_values_size(values) != 1) {
		SLV2_ERRORF("Port does not have exactly one `%s' property\n",
		            sord_node_get_string(predicate));
		return NULL;
	}
	SLV2Value ret = slv2_value_duplicate(slv2_values_get_at(values, 0));
	slv2_values_free(values);
	return ret;
}

static SLV2Value
slv2_plugin_get_one(SLV2Plugin p, SLV2Node subject, SLV2Node predicate)
{
	SLV2Values values = slv2_plugin_query_node(p, subject, predicate);
	if (!values) {
		return NULL;
	}
	SLV2Value ret = slv2_value_duplicate(slv2_values_get_at(values, 0));
	slv2_values_free(values);
	return ret;
}
	
	
/* private */
void
slv2_plugin_load_ports_if_necessary(SLV2Plugin p)
{
	if (!p->loaded)
		slv2_plugin_load(p);

	if (!p->ports) {
		p->ports = malloc(sizeof(SLV2Port*));
		p->ports[0] = NULL;

		SLV2Matches ports = slv2_plugin_find_statements(
			p,
			p->plugin_uri->val.uri_val,
			p->world->lv2_port_node,
			NULL);

		FOREACH_MATCH(ports) {
			SLV2Node  port   = slv2_match_object(ports);
			SLV2Value symbol = slv2_plugin_get_unique(
				p, port, p->world->lv2_symbol_node);

			if (!slv2_value_is_string(symbol)) {
				SLV2_ERROR("port has a non-string symbol\n");
				p->num_ports = 0;
				goto error;
			}

			SLV2Value index = slv2_plugin_get_unique(
				p, port, p->world->lv2_index_node);

			if (!slv2_value_is_int(index)) {
				SLV2_ERROR("port has a non-integer index\n");
				p->num_ports = 0;
				goto error;
			}

			uint32_t this_index = slv2_value_as_int(index);
			SLV2Port this_port  = NULL;
			if (p->num_ports > this_index) {
				this_port = p->ports[this_index];
			} else {
				p->ports = realloc(p->ports, (this_index + 1) * sizeof(SLV2Port*));
				memset(p->ports + p->num_ports, '\0',
				       (this_index - p->num_ports) * sizeof(SLV2Port));
				p->num_ports = this_index + 1;
			}

			// Havn't seen this port yet, add it to array
			if (!this_port) {
				this_port = slv2_port_new(p->world,
				                          this_index,
				                          slv2_value_as_string(symbol));
				p->ports[this_index] = this_port;
			}

			SLV2Matches types = slv2_plugin_find_statements(
				p, port, p->world->rdf_a_node, NULL);
			FOREACH_MATCH(types) {
				SLV2Node type = slv2_match_object(types);
				if (sord_node_get_type(type) == SORD_URI) {
					g_ptr_array_add(
						this_port->classes,
						slv2_value_new_from_node(p->world, type));
				} else {
					SLV2_WARN("port has non-URI rdf:type\n");
				}
			}
			slv2_match_end(types);

		error:
			slv2_value_free(symbol);
			slv2_value_free(index);
			if (p->num_ports == 0) {
				if (p->ports) {
					for (uint32_t i = 0; i < p->num_ports; ++i)
						slv2_port_free(p->ports[i]);
					free(p->ports);
					p->ports = NULL;
				}
				break; // Invalid plugin
			}
		}
		slv2_match_end(ports);
	}
}

void
slv2_plugin_load(SLV2Plugin p)
{
	// Parse all the plugin's data files into RDF model
	for (unsigned i = 0; i < slv2_values_size(p->data_uris); ++i) {
		SLV2Value data_uri_val = slv2_values_get_at(p->data_uris, i);
		sord_read_file(p->world->model,
		               sord_node_get_string(data_uri_val->val.uri_val),
		               p->bundle_uri->val.uri_val,
		               slv2_world_blank_node_prefix(p->world));
	}

#ifdef SLV2_DYN_MANIFEST
	typedef void* LV2_Dyn_Manifest_Handle;
	// Load and parse dynamic manifest data, if this is a library
	if (p->dynman_uri) {
		const char* lib_path = slv2_uri_to_path(slv2_value_as_string(p->dynman_uri));
		void* lib = dlopen(lib_path, RTLD_LAZY);
		if (!lib) {
			SLV2_WARNF("Unable to open dynamic manifest %s\n", slv2_value_as_string(p->dynman_uri));
			return;
		}

		typedef int (*OpenFunc)(LV2_Dyn_Manifest_Handle*, const LV2_Feature *const *);
		OpenFunc open_func = (OpenFunc)dlsym(lib, "lv2_dyn_manifest_open");
		LV2_Dyn_Manifest_Handle handle = NULL;
		if (open_func)
			open_func(&handle, &dman_features);

		typedef int (*GetDataFunc)(LV2_Dyn_Manifest_Handle handle,
		                           FILE*                   fp,
		                           const char*             uri);
		GetDataFunc get_data_func = (GetDataFunc)dlsym(lib, "lv2_dyn_manifest_get_data");
		if (get_data_func) {
			FILE* fd = tmpfile();
			get_data_func(handle, fd, slv2_value_as_string(p->plugin_uri));
			rewind(fd);
			sord_read_file_handle(p->world->model, fd, p->bundle_uri);
			fclose(fd);
		}

		typedef int (*CloseFunc)(LV2_Dyn_Manifest_Handle);
		CloseFunc close_func = (CloseFunc)dlsym(lib, "lv2_dyn_manifest_close");
		if (close_func)
			close_func(handle);
	}
#endif
	p->loaded = true;
}

SLV2Value
slv2_plugin_get_uri(SLV2Plugin p)
{
	assert(p);
	assert(p->plugin_uri);
	return p->plugin_uri;
}

SLV2Value
slv2_plugin_get_bundle_uri(SLV2Plugin p)
{
	assert(p);
	assert(p->bundle_uri);
	return p->bundle_uri;
}

SLV2Value
slv2_plugin_get_library_uri(SLV2Plugin p)
{
	slv2_plugin_load_if_necessary(p);
	if (!p->binary_uri) {
		// <plugin> lv2:binary ?binary
		SLV2Matches results = slv2_plugin_find_statements(
			p,
			p->plugin_uri->val.uri_val,
			p->world->lv2_binary_node,
			NULL);
		FOREACH_MATCH(results) {
			SLV2Node binary_node = slv2_match_object(results);
			if (sord_node_get_type(binary_node) == SORD_URI) {
				p->binary_uri = slv2_value_new_from_node(p->world, binary_node);
				break;
			}
		}
		slv2_match_end(results);
	}
	return p->binary_uri;
}

SLV2Values
slv2_plugin_get_data_uris(SLV2Plugin p)
{
	return p->data_uris;
}

SLV2PluginClass
slv2_plugin_get_class(SLV2Plugin p)
{
	slv2_plugin_load_if_necessary(p);
	if (!p->plugin_class) {
		// <plugin> a ?class
		SLV2Matches results = slv2_plugin_find_statements(
			p,
			p->plugin_uri->val.uri_val,
			p->world->rdf_a_node,
			NULL);
		FOREACH_MATCH(results) {
			SLV2Node class_node = slv2_node_copy(slv2_match_object(results));
			if (sord_node_get_type(class_node) != SORD_URI) {
				continue;
			}

			SLV2Value class = slv2_value_new_from_node(p->world, class_node);
			if ( ! slv2_value_equals(class, p->world->lv2_plugin_class->uri)) {

				SLV2PluginClass plugin_class = slv2_plugin_classes_get_by_uri(
						p->world->plugin_classes, class);

				slv2_node_free(class_node);

				if (plugin_class) {
					p->plugin_class = plugin_class;
					slv2_value_free(class);
					break;
				}
			}

			slv2_value_free(class);
		}
		slv2_match_end(results);

		if (p->plugin_class == NULL)
			p->plugin_class = p->world->lv2_plugin_class;
	}
	return p->plugin_class;
}

bool
slv2_plugin_verify(SLV2Plugin plugin)
{
	SLV2Values results = slv2_plugin_get_value_by_qname(plugin, "rdf:type");
	if (!results) {
		return false;
	}

	slv2_values_free(results);
	results = slv2_plugin_get_value_by_qname(plugin, "doap:name");
	if (!results) {
		return false;
	}

	slv2_values_free(results);
	results = slv2_plugin_get_value_by_qname(plugin, "doap:license");
	if (!results) {
		return false;
	}

	slv2_values_free(results);
	results = slv2_plugin_get_value_by_qname(plugin, "lv2:port");
	if (!results) {
		return false;
	}

	slv2_values_free(results);
	return true;
}

SLV2Value
slv2_plugin_get_name(SLV2Plugin plugin)
{
	SLV2Values results = slv2_plugin_get_value_by_qname_i18n(plugin, "doap:name");
	SLV2Value  ret     = NULL;

	if (results) {
		SLV2Value val = slv2_values_get_at(results, 0);
		if (slv2_value_is_string(val))
			ret = slv2_value_duplicate(val);
		slv2_values_free(results);
	} else {
		results = slv2_plugin_get_value_by_qname(plugin, "doap:name");
		SLV2Value val = slv2_values_get_at(results, 0);
		if (slv2_value_is_string(val))
			ret = slv2_value_duplicate(val);
		slv2_values_free(results);
	}

	if (!ret)
		SLV2_WARNF("<%s> has no (mandatory) doap:name\n",
				slv2_value_as_string(slv2_plugin_get_uri(plugin)));

	return ret;
}

SLV2Values
slv2_plugin_get_value(SLV2Plugin p,
                      SLV2Value  predicate)
{
	return slv2_plugin_get_value_for_subject(p, p->plugin_uri, predicate);
}

SLV2Values
slv2_plugin_get_value_by_qname(SLV2Plugin  p,
                               const char* predicate)
{
	char* pred_uri = slv2_qname_expand(p, predicate);
	if (!pred_uri) {
		return NULL;
	}
	SLV2Value  pred_value = slv2_value_new_uri(p->world, pred_uri);
	SLV2Values ret        = slv2_plugin_get_value(p, pred_value);

	slv2_value_free(pred_value);
	free(pred_uri);
	return ret;
}

SLV2Values
slv2_plugin_get_value_by_qname_i18n(SLV2Plugin  p,
                                    const char* predicate)
{
	char* pred_uri = slv2_qname_expand(p, predicate);
	if (!pred_uri) {
		return NULL;
	}

	SLV2Node pred_node = sord_get_uri(
		p->world->model, true, (const uint8_t*)pred_uri);

	SLV2Matches results = slv2_plugin_find_statements(
		p,
		p->plugin_uri->val.uri_val,
		pred_node,
		NULL);

	slv2_node_free(pred_node);
	free(pred_uri);
	return slv2_values_from_stream_i18n(p, results);
}

SLV2Values
slv2_plugin_get_value_for_subject(SLV2Plugin p,
                                  SLV2Value  subject,
                                  SLV2Value  predicate)
{
	if ( ! slv2_value_is_uri(subject) && ! slv2_value_is_blank(subject)) {
		SLV2_ERROR("Subject is not a resource\n");
		return NULL;
	}
	if ( ! slv2_value_is_uri(predicate)) {
		SLV2_ERROR("Predicate is not a URI\n");
		return NULL;
	}

	SLV2Node subject_node = (slv2_value_is_uri(subject))
		? slv2_node_copy(subject->val.uri_val)
		: sord_get_blank(p->world->model, false,
		                 (const uint8_t*)slv2_value_as_blank(subject));

	if (!subject_node) {
		fprintf(stderr, "No such subject\n");
		return NULL;
	}

	return slv2_plugin_query_node(p,
	                              subject_node,
	                              predicate->val.uri_val);
}

uint32_t
slv2_plugin_get_num_ports(SLV2Plugin p)
{
	slv2_plugin_load_ports_if_necessary(p);
	return p->num_ports;
}

void
slv2_plugin_get_port_ranges_float(SLV2Plugin p,
                                  float*     min_values,
                                  float*     max_values,
                                  float*     def_values)
{
	slv2_plugin_load_ports_if_necessary(p);
	for (unsigned i = 0; i < p->num_ports; ++i) {
		SLV2Port port = p->ports[i];
		SLV2Value def, min, max;
		slv2_port_get_range(p, port, &def, &min, &max);
		
		if (min && min_values)
			min_values[i] = slv2_value_as_float(min);

		if (max && max_values)
			max_values[i] = slv2_value_as_float(max);

		if (def && def_values)
			def_values[i] = slv2_value_as_float(def);

		slv2_value_free(def);
		slv2_value_free(min);
		slv2_value_free(max);
	}
}

uint32_t
slv2_plugin_get_num_ports_of_class(SLV2Plugin p,
                                   SLV2Value  class_1, ...)
{
	slv2_plugin_load_ports_if_necessary(p);

	uint32_t ret = 0;
	va_list  args;

	for (unsigned i = 0; i < p->num_ports; ++i) {
		SLV2Port port = p->ports[i];
		if (!port || !slv2_port_is_a(p, port, class_1))
			continue;

		va_start(args, class_1);

		bool matches = true;
		for (SLV2Value class_i = NULL; (class_i = va_arg(args, SLV2Value)) != NULL ; ) {
			if (!slv2_port_is_a(p, port, class_i)) {
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

bool
slv2_plugin_has_latency(SLV2Plugin p)
{
	SLV2Matches ports = slv2_plugin_find_statements(
		p,
		p->plugin_uri->val.uri_val,
		p->world->lv2_port_node,
		NULL);

	bool ret = false;
	FOREACH_MATCH(ports) {
		SLV2Node    port            = slv2_match_object(ports);
		SLV2Matches reports_latency = slv2_plugin_find_statements(
			p,
			port,
			p->world->lv2_portproperty_node,
			p->world->lv2_reportslatency_node);
		const bool end = slv2_matches_end(reports_latency);
		slv2_match_end(reports_latency);
		if (!end) {
			ret = true;
			break;
		}
	}
	slv2_match_end(ports);

	return ret;
}

uint32_t
slv2_plugin_get_latency_port_index(SLV2Plugin p)
{
	SLV2Matches ports = slv2_plugin_find_statements(
		p,
		p->plugin_uri->val.uri_val,
		p->world->lv2_port_node,
		NULL);

	uint32_t ret = 0;
	FOREACH_MATCH(ports) {
		SLV2Node    port            = slv2_match_object(ports);
		SLV2Matches reports_latency = slv2_plugin_find_statements(
			p,
			port,
			p->world->lv2_portproperty_node,
			p->world->lv2_reportslatency_node);
		if (!slv2_matches_end(reports_latency)) {
			SLV2Value index = slv2_plugin_get_unique(
				p, port, p->world->lv2_index_node);

			ret = slv2_value_as_int(index);
			slv2_value_free(index);
			slv2_match_end(reports_latency);
			break;
		}
		slv2_match_end(reports_latency);
	}
	slv2_match_end(ports);

	return ret;  // FIXME: error handling
}

bool
slv2_plugin_has_feature(SLV2Plugin p,
                        SLV2Value  feature)
{
	SLV2Values features = slv2_plugin_get_supported_features(p);

	const bool ret = features && feature && slv2_values_contains(features, feature);

	slv2_values_free(features);
	return ret;
}

SLV2Values
slv2_plugin_get_supported_features(SLV2Plugin p)
{
	SLV2Values optional   = slv2_plugin_get_optional_features(p);
	SLV2Values required   = slv2_plugin_get_required_features(p);
	SLV2Values result     = slv2_values_new();
	unsigned   n_optional = slv2_values_size(optional);
	unsigned   n_required = slv2_values_size(required);
	for (unsigned i = 0 ; i < n_optional; ++i)
		g_ptr_array_add(result, slv2_values_get_at(optional, i));
	for (unsigned i = 0 ; i < n_required; ++i)
		g_ptr_array_add(result, slv2_values_get_at(required, i));

	free(((GPtrArray*)optional)->pdata);
	free(((GPtrArray*)required)->pdata);

	return result;
}

SLV2Values
slv2_plugin_get_optional_features(SLV2Plugin p)
{
	return slv2_plugin_get_value_by_qname(p, "lv2:optionalFeature");
}

SLV2Values
slv2_plugin_get_required_features(SLV2Plugin p)
{
	return slv2_plugin_get_value_by_qname(p, "lv2:requiredFeature");
}

SLV2Port
slv2_plugin_get_port_by_index(SLV2Plugin p,
                              uint32_t   index)
{
	slv2_plugin_load_ports_if_necessary(p);
	if (index < p->num_ports)
		return p->ports[index];
	else
		return NULL;
}

SLV2Port
slv2_plugin_get_port_by_symbol(SLV2Plugin p,
                               SLV2Value  symbol)
{
	slv2_plugin_load_ports_if_necessary(p);
	for (uint32_t i = 0; i < p->num_ports; ++i) {
		SLV2Port port = p->ports[i];
		if (slv2_value_equals(port->symbol, symbol))
			return port;
	}

	return NULL;
}

#define NS_DOAP (const uint8_t*)"http://usefulinc.com/ns/doap#"
#define NS_FOAF (const uint8_t*)"http://xmlns.com/foaf/0.1/"

static SLV2Node
slv2_plugin_get_author(SLV2Plugin p)
{
	SLV2Node doap_maintainer = sord_get_uri(
		p->world->model, true, NS_DOAP "maintainer");

	SLV2Matches maintainers = slv2_plugin_find_statements(
		p,
		p->plugin_uri->val.uri_val,
		doap_maintainer,
		NULL);

	slv2_node_free(doap_maintainer);

	if (slv2_matches_end(maintainers)) {
		return NULL;
	}

	SLV2Node author = slv2_node_copy(slv2_match_object(maintainers));

	slv2_match_end(maintainers);
	return author;
}

SLV2Value
slv2_plugin_get_author_name(SLV2Plugin plugin)
{
	SLV2Node author = slv2_plugin_get_author(plugin);
	if (author) {
		return slv2_plugin_get_one(
			plugin, author, sord_get_uri(
				plugin->world->model, true, NS_FOAF "name"));
	}
	return NULL;
}

SLV2Value
slv2_plugin_get_author_email(SLV2Plugin plugin)
{
	SLV2Node author = slv2_plugin_get_author(plugin);
	if (author) {
		return slv2_plugin_get_one(
			plugin, author, sord_get_uri(
				plugin->world->model, true, NS_FOAF "mbox"));
	}
	return NULL;
}

SLV2Value
slv2_plugin_get_author_homepage(SLV2Plugin plugin)
{
	SLV2Node author = slv2_plugin_get_author(plugin);
	if (author) {
		return slv2_plugin_get_one(
			plugin, author, sord_get_uri(
				plugin->world->model, true, NS_FOAF "homepage"));
	}
	return NULL;
}

SLV2UIs
slv2_plugin_get_uis(SLV2Plugin p)
{
#define NS_UI (const uint8_t*)"http://lv2plug.in/ns/extensions/ui#"

	SLV2Node ui_ui = sord_get_uri(
		p->world->model, true, NS_UI "ui");

	SLV2Node ui_binary_node = sord_get_uri(
		p->world->model, true, NS_UI "binary");

	SLV2UIs     result = slv2_uis_new();
	SLV2Matches uis    = slv2_plugin_find_statements(
		p,
		p->plugin_uri->val.uri_val,
		ui_ui,
		NULL);

	FOREACH_MATCH(uis) {
		SLV2Node  ui     = slv2_match_object(uis);
		SLV2Value type   = slv2_plugin_get_unique(p, ui, p->world->rdf_a_node);
		SLV2Value binary = slv2_plugin_get_unique(p, ui, ui_binary_node);

		if (sord_node_get_type(ui) != SORD_URI
		    || !slv2_value_is_uri(type)
		    || !slv2_value_is_uri(binary)) {
			slv2_value_free(binary);
			slv2_value_free(type);
			SLV2_ERROR("Corrupt UI\n");
			continue;
		}
		
		SLV2UI slv2_ui = slv2_ui_new(
			p->world,
			slv2_value_new_from_node(p->world, ui),
			type,
			binary);

		g_ptr_array_add(result, slv2_ui);
	}
	slv2_match_end(uis);

	slv2_node_free(ui_binary_node);
	slv2_node_free(ui_ui);

	if (slv2_uis_size(result) > 0) {
		return result;
	} else {
		slv2_uis_free(result);
		return NULL;
	}
}

