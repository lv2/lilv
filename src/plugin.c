/* SLV2
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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
#include <redland.h>
#include "slv2/types.h"
#include "slv2/collections.h"
#include "slv2/plugin.h"
#include "slv2/pluginclass.h"
#include "slv2/query.h"
#include "slv2/util.h"
#include "slv2_internal.h"


/* private
 * ownership of uri is taken */
SLV2Plugin
slv2_plugin_new(SLV2World world, SLV2Value uri, librdf_uri* bundle_uri)
{
	assert(bundle_uri);
	struct _SLV2Plugin* plugin = malloc(sizeof(struct _SLV2Plugin));
	plugin->world = world;
	plugin->plugin_uri = uri;
	plugin->bundle_uri = slv2_value_new_librdf_uri(world, bundle_uri);
	plugin->binary_uri = NULL;
#ifdef SLV2_DYN_MANIFEST
	plugin->dynman_uri = NULL;
#endif
	plugin->plugin_class = NULL;
	plugin->data_uris = slv2_values_new();
	plugin->ports = NULL;
	plugin->storage = NULL;
	plugin->rdf = NULL;
	plugin->num_ports = 0;

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

	if (p->rdf) {
		librdf_free_model(p->rdf);
		p->rdf = NULL;
	}

	if (p->storage) {
		librdf_free_storage(p->storage);
		p->storage = NULL;
	}

	slv2_values_free(p->data_uris);
	p->data_uris = NULL;

	free(p);
}


/* private */
void
slv2_plugin_load_if_necessary(SLV2Plugin p)
{
	if (!p->rdf)
		slv2_plugin_load(p);
}


static SLV2Values
slv2_plugin_query_node(SLV2Plugin p, librdf_node* subject, librdf_node* predicate)
{
	// <subject> <predicate> ?value
	librdf_stream* results = slv2_plugin_find_statements(
		p, subject, predicate, NULL);

	if (librdf_stream_end(results)) {
		return NULL;
	}

	SLV2Values result = slv2_values_new();
	FOREACH_MATCH(results) {
		librdf_statement* s          = librdf_stream_get_object(results);
		librdf_node*      value_node = librdf_statement_get_object(s);

		SLV2Value value = slv2_value_new_librdf_node(p->world, value_node);
		if (value)
			raptor_sequence_push(result, value);
	}
	END_MATCH(results);

	return result;
}


SLV2Value
slv2_plugin_get_unique(SLV2Plugin p, librdf_node* subject, librdf_node* predicate)
{
	SLV2Values values = slv2_plugin_query_node(p, subject, predicate);
	if (!values || slv2_values_size(values) != 1) {
		SLV2_ERRORF("Port does not have exactly one `%s' property\n",
		            librdf_uri_as_string(librdf_node_get_uri(predicate)));
		return NULL;
	}
	SLV2Value ret = slv2_value_duplicate(slv2_values_get_at(values, 0));
	slv2_values_free(values);
	return ret;
}


static SLV2Value
slv2_plugin_get_one(SLV2Plugin p, librdf_node* subject, librdf_node* predicate)
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
	if (!p->rdf)
		slv2_plugin_load(p);

	if (!p->ports) {
		p->ports = malloc(sizeof(SLV2Port*));
		p->ports[0] = NULL;

		librdf_stream* ports = slv2_plugin_find_statements(
			p,
			librdf_new_node_from_uri(p->world->world, p->plugin_uri->val.uri_val),
			librdf_new_node_from_node(p->world->lv2_port_node),
			NULL);

		FOREACH_MATCH(ports) {
			librdf_statement* s    = librdf_stream_get_object(ports);
			librdf_node*      port = librdf_statement_get_object(s);

			SLV2Value symbol = slv2_plugin_get_unique(
				p,
				librdf_new_node_from_node(port),
				librdf_new_node_from_node(p->world->lv2_symbol_node));

			if (!slv2_value_is_string(symbol)) {
				SLV2_ERROR("port has a non-string symbol\n");
				p->num_ports = 0;
				goto error;
			}

			SLV2Value index = slv2_plugin_get_unique(
				p,
				librdf_new_node_from_node(port),
				librdf_new_node_from_node(p->world->lv2_index_node));

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

			librdf_stream* types = slv2_plugin_find_statements(
				p,
				librdf_new_node_from_node(port),
				librdf_new_node_from_node(p->world->rdf_a_node),
				NULL);
			FOREACH_MATCH(types) {
				librdf_node* type = librdf_statement_get_object(
					librdf_stream_get_object(types));
				if (librdf_node_is_resource(type)) {
					raptor_sequence_push(
						this_port->classes,
						slv2_value_new_librdf_uri(p->world, librdf_node_get_uri(type)));
				} else {
					SLV2_WARN("port has non-URI rdf:type\n");
				}
			}
			END_MATCH(types);

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
		END_MATCH(ports);
	}
}


void
slv2_plugin_load(SLV2Plugin p)
{
	if (!p->storage) {
		assert(!p->rdf);
		p->storage = slv2_world_new_storage(p->world);
		p->rdf = librdf_new_model(p->world->world, p->storage, NULL);
	}

	// Parse all the plugin's data files into RDF model
	for (unsigned i=0; i < slv2_values_size(p->data_uris); ++i) {
		SLV2Value data_uri_val = slv2_values_get_at(p->data_uris, i);
		librdf_uri* data_uri = slv2_value_as_librdf_uri(data_uri_val);
		librdf_parser_parse_into_model(p->world->parser, data_uri, data_uri, p->rdf);
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
			librdf_parser_parse_file_handle_into_model(p->world->parser,
					fd, 0, slv2_value_as_librdf_uri(p->bundle_uri), p->rdf);
			fclose(fd);
		}

		typedef int (*CloseFunc)(LV2_Dyn_Manifest_Handle);
		CloseFunc close_func = (CloseFunc)dlsym(lib, "lv2_dyn_manifest_close");
		if (close_func)
			close_func(handle);
	}
#endif
	assert(p->rdf);
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
		librdf_stream* results = slv2_plugin_find_statements(
			p,
			librdf_new_node_from_uri(p->world->world, p->plugin_uri->val.uri_val),
			librdf_new_node_from_node(p->world->lv2_binary_node),
			NULL);
		FOREACH_MATCH(results) {
			librdf_statement* s           = librdf_stream_get_object(results);
			librdf_node*      binary_node = librdf_statement_get_object(s);
			librdf_uri*       binary_uri  = librdf_node_get_uri(binary_node);

			if (binary_uri) {
				p->binary_uri = slv2_value_new_librdf_uri(p->world, binary_uri);
				break;
			}
		}
		END_MATCH(results);
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
		librdf_stream* results = slv2_plugin_find_statements(
			p,
			librdf_new_node_from_uri(p->world->world, p->plugin_uri->val.uri_val),
			librdf_new_node_from_node(p->world->rdf_a_node),
			NULL);
		FOREACH_MATCH(results) {
			librdf_statement* s          = librdf_stream_get_object(results);
			librdf_node*      class_node = librdf_new_node_from_node(librdf_statement_get_object(s));
			librdf_uri*       class_uri  = librdf_node_get_uri(class_node);

			if (!class_uri) {
				continue;
			}

			SLV2Value class = slv2_value_new_librdf_uri(p->world, class_uri);

			if ( ! slv2_value_equals(class, p->world->lv2_plugin_class->uri)) {

				SLV2PluginClass plugin_class = slv2_plugin_classes_get_by_uri(
						p->world->plugin_classes, class);

				librdf_free_node(class_node);

				if (plugin_class) {
					p->plugin_class = plugin_class;
					slv2_value_free(class);
					break;
				}
			}

			slv2_value_free(class);
		}
		END_MATCH(results);

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

	librdf_node* pred_node = librdf_new_node_from_uri_string(
		p->world->world, (const uint8_t*)pred_uri);

	librdf_stream* results = slv2_plugin_find_statements(
		p,
		librdf_new_node_from_uri(p->world->world, p->plugin_uri->val.uri_val),
		pred_node,
		NULL);

	free(pred_uri);

	return slv2_values_from_stream_i18n(p, results);
}


SLV2Values
slv2_plugin_get_value_for_subject(SLV2Plugin p,
                                  SLV2Value  subject,
                                  SLV2Value  predicate)
{
	if ( ! slv2_value_is_uri(subject)) {
		SLV2_ERROR("Subject is not a URI\n");
		return NULL;
	}
	if ( ! slv2_value_is_uri(predicate)) {
		SLV2_ERROR("Predicate is not a URI\n");
		return NULL;
	}

	return slv2_plugin_query_node(
		p,
		librdf_new_node_from_uri(p->world->world, subject->val.uri_val),
		librdf_new_node_from_uri(p->world->world, predicate->val.uri_val));
}


SLV2Values
slv2_plugin_get_properties(SLV2Plugin p)
{
	// FIXME: APIBREAK: This predicate does not even exist.  Remove this function.
	return slv2_plugin_get_value_by_qname(p, "lv2:pluginProperty");
}


SLV2Values
slv2_plugin_get_hints(SLV2Plugin p)
{
	// FIXME: APIBREAK: This predicate does not even exist.  Remove this function.
	return slv2_plugin_get_value_by_qname(p, "lv2:pluginHint");
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
	}
}


uint32_t
slv2_plugin_get_num_ports_of_class(SLV2Plugin p,
                                   SLV2Value  class_1, ...)
{
	slv2_plugin_load_ports_if_necessary(p);

	uint32_t ret = 0;
	va_list  args;

	for (unsigned i=0; i < p->num_ports; ++i) {
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
	librdf_stream* ports = slv2_plugin_find_statements(
		p,
		librdf_new_node_from_uri(p->world->world, p->plugin_uri->val.uri_val),
		librdf_new_node_from_node(p->world->lv2_port_node),
		NULL);

	bool ret = false;
	FOREACH_MATCH(ports) {
		librdf_statement* s    = librdf_stream_get_object(ports);
		librdf_node*      port = librdf_statement_get_object(s);

		librdf_stream* reports_latency = slv2_plugin_find_statements(
			p,
			librdf_new_node_from_node(port),
			librdf_new_node_from_node(p->world->lv2_portproperty_node),
			librdf_new_node_from_uri_string(p->world->world,
			                                SLV2_NS_LV2 "reportsLatency"));

		if (!librdf_stream_end(reports_latency)) {
			ret = true;
			break;
		}

		librdf_free_stream(reports_latency);
	}
	END_MATCH(ports);

	return ret;
}


uint32_t
slv2_plugin_get_latency_port_index(SLV2Plugin p)
{
	librdf_stream* ports = slv2_plugin_find_statements(
		p,
		librdf_new_node_from_uri(p->world->world, p->plugin_uri->val.uri_val),
		librdf_new_node_from_node(p->world->lv2_port_node),
		NULL);

	uint32_t ret = 0;
	FOREACH_MATCH(ports) {
		librdf_statement* s    = librdf_stream_get_object(ports);
		librdf_node*      port = librdf_statement_get_object(s);

		librdf_stream* reports_latency = slv2_plugin_find_statements(
			p,
			librdf_new_node_from_node(port),
			librdf_new_node_from_node(p->world->lv2_portproperty_node),
			librdf_new_node_from_uri_string(p->world->world,
			                                SLV2_NS_LV2 "reportsLatency"));

		if (!librdf_stream_end(reports_latency)) {
			SLV2Value index = slv2_plugin_get_unique(
				p,
				librdf_new_node_from_node(port),
				librdf_new_node_from_node(p->world->lv2_index_node));
			ret = slv2_value_as_int(index);
			slv2_value_free(index);
			break;
		}
	}
	END_MATCH(ports);

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
	SLV2Values optional = slv2_plugin_get_optional_features(p);
	SLV2Values required = slv2_plugin_get_required_features(p);

	SLV2Values result = slv2_values_new();
	unsigned n_optional = slv2_values_size(optional);
	unsigned n_required = slv2_values_size(required);
	unsigned i = 0;
	for ( ; i < n_optional; ++i)
		slv2_values_set_at(result, i, raptor_sequence_pop(optional));
	for ( ; i < n_optional + n_required; ++i)
		slv2_values_set_at(result, i, raptor_sequence_pop(required));

	slv2_values_free(optional);
	slv2_values_free(required);

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
	for (uint32_t i=0; i < p->num_ports; ++i) {
		SLV2Port port = p->ports[i];
		if (slv2_value_equals(port->symbol, symbol))
			return port;
	}

	return NULL;
}

#define NS_DOAP (const uint8_t*)"http://usefulinc.com/ns/doap#"
#define NS_FOAF (const uint8_t*)"http://xmlns.com/foaf/0.1/"

static librdf_node*
slv2_plugin_get_author(SLV2Plugin p)
{

	librdf_stream* maintainers = slv2_plugin_find_statements(
		p,
		librdf_new_node_from_uri(p->world->world, p->plugin_uri->val.uri_val),
		librdf_new_node_from_uri_string(p->world->world, NS_DOAP "maintainer"),
		NULL);

	if (librdf_stream_end(maintainers)) {
		return NULL;
	}

	librdf_node* author = librdf_new_node_from_node(
		librdf_statement_get_object(librdf_stream_get_object(maintainers)));

	librdf_free_stream(maintainers);
	return author;
}


SLV2Value
slv2_plugin_get_author_name(SLV2Plugin plugin)
{
	librdf_node* author = slv2_plugin_get_author(plugin);
	if (author) {
		return slv2_plugin_get_one(
			plugin, author, librdf_new_node_from_uri_string(
				plugin->world->world, NS_FOAF "name"));
	}
	return NULL;
}


SLV2Value
slv2_plugin_get_author_email(SLV2Plugin plugin)
{
	librdf_node* author = slv2_plugin_get_author(plugin);
	if (author) {
		return slv2_plugin_get_one(
			plugin, author, librdf_new_node_from_uri_string(
				plugin->world->world, NS_FOAF "mbox"));
	}
	return NULL;
}


SLV2Value
slv2_plugin_get_author_homepage(SLV2Plugin plugin)
{
	librdf_node* author = slv2_plugin_get_author(plugin);
	if (author) {
		return slv2_plugin_get_one(
			plugin, author, librdf_new_node_from_uri_string(
				plugin->world->world, NS_FOAF "homepage"));
	}
	return NULL;
}


SLV2UIs
slv2_plugin_get_uis(SLV2Plugin p)
{
#define NS_UI (const uint8_t*)"http://lv2plug.in/ns/extensions/ui#"

	SLV2UIs        result = slv2_uis_new();
	librdf_stream* uis    = slv2_plugin_find_statements(
		p,
		librdf_new_node_from_uri(p->world->world, p->plugin_uri->val.uri_val),
		librdf_new_node_from_uri_string(p->world->world, NS_UI "ui"),
		NULL);
	FOREACH_MATCH(uis) {
		librdf_statement* s  = librdf_stream_get_object(uis);
		librdf_node*      ui = librdf_statement_get_object(s);

		SLV2Value type = slv2_plugin_get_unique(
			p,
			librdf_new_node_from_node(ui),
			librdf_new_node_from_node(p->world->rdf_a_node));

		SLV2Value binary = slv2_plugin_get_unique(
			p,
			librdf_new_node_from_node(ui),
			librdf_new_node_from_uri_string(p->world->world, NS_UI "binary"));

		if (!librdf_node_is_resource(ui)
		    || !slv2_value_is_uri(type)
		    || !slv2_value_is_uri(binary)) {
			SLV2_ERROR("Corrupt UI\n");
			continue;
		}
		
		SLV2UI slv2_ui = slv2_ui_new(p->world,
		                             librdf_node_get_uri(ui),
		                             slv2_value_as_librdf_uri(type),
		                             slv2_value_as_librdf_uri(binary));

		raptor_sequence_push(result, slv2_ui);

		slv2_value_free(binary);
		slv2_value_free(type);
	}
	END_MATCH(uis);

	if (slv2_uis_size(result) > 0) {
		return result;
	} else {
		slv2_uis_free(result);
		return NULL;
	}
}

