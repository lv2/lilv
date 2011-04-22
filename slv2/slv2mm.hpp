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

#ifndef SLV2_SLV2MM_HPP
#define SLV2_SLV2MM_HPP

#include "slv2/slv2.h"

namespace SLV2 {

const char* uri_to_path(const char* uri) { return slv2_uri_to_path(uri); }

#define SLV2_WRAP0(RT, prefix, name) \
	inline RT name () { return slv2_ ## prefix ## _ ## name (me); }

#define SLV2_WRAP0_VOID(prefix, name) \
	inline void name () { slv2_ ## prefix ## _ ## name (me); }

#define SLV2_WRAP1(RT, prefix, name, T1, a1) \
	inline RT name (T1 a1) { return slv2_ ## prefix ## _ ## name (me, a1); }

#define SLV2_WRAP1_VOID(prefix, name, T1, a1) \
	inline void name (T1 a1) { slv2_ ## prefix ## _ ## name (me, a1); }

#define SLV2_WRAP2(RT, prefix, name, T1, a1, T2, a2) \
	inline RT name (T1 a1, T2 a2) { return slv2_ ## prefix ## _ ## name (me, a1, a2); }

#define SLV2_WRAP2_VOID(prefix, name, T1, a1, T2, a2) \
	inline void name (T1 a1, T2 a2) { slv2_ ## prefix ## _ ## name (me, a1, a2); }

struct Value {
	inline Value(SLV2Value value)   : me(value) {}
	inline Value(const Value& copy) : me(slv2_value_duplicate(copy.me)) {}

	inline ~Value() { /*slv2_value_free(me);*/ }

	inline bool equals(const Value& other) const {
		return slv2_value_equals(me, other.me);
	}

	inline bool operator==(const Value& other) const { return equals(other); }

	inline operator SLV2Value() const { return me; }

	SLV2_WRAP0(char*,       value, get_turtle_token);
	SLV2_WRAP0(bool,        value, is_uri);
	SLV2_WRAP0(const char*, value, as_uri);
	SLV2_WRAP0(bool,        value, is_blank);
	SLV2_WRAP0(const char*, value, as_blank);
	SLV2_WRAP0(bool,        value, is_literal);
	SLV2_WRAP0(bool,        value, is_string);
	SLV2_WRAP0(const char*, value, as_string);
	SLV2_WRAP0(bool,        value, is_float);
	SLV2_WRAP0(float,       value, as_float);
	SLV2_WRAP0(bool,        value, is_int);
	SLV2_WRAP0(int,         value, as_int);
	SLV2_WRAP0(bool,        value, is_bool);
	SLV2_WRAP0(bool,        value, as_bool);

	SLV2Value me;
};

struct ScalePoint {
	inline ScalePoint(SLV2ScalePoint c_obj) : me(c_obj) {}
	inline operator SLV2ScalePoint() const { return me; }

	SLV2_WRAP0(SLV2Value, scale_point, get_label);
	SLV2_WRAP0(SLV2Value, scale_point, get_value);

	SLV2ScalePoint me;
};

struct PluginClass {
	inline PluginClass(SLV2PluginClass c_obj) : me(c_obj) {}
	inline operator SLV2PluginClass() const { return me; }

	SLV2_WRAP0(Value, plugin_class, get_parent_uri);
	SLV2_WRAP0(Value, plugin_class, get_uri);
	SLV2_WRAP0(Value, plugin_class, get_label);

	inline SLV2PluginClasses get_children() {
		return slv2_plugin_class_get_children(me);
	}

	SLV2PluginClass me;
};

#define SLV2_WRAP_COLL(CT, ET, prefix) \
	struct CT { \
		inline CT(SLV2 ## CT c_obj) : me(c_obj) {} \
		inline operator SLV2 ## CT () const { return me; } \
		SLV2_WRAP0(unsigned, prefix, size); \
		SLV2_WRAP1(ET, prefix, get_at, unsigned, index); \
		SLV2 ## CT me; \
	}; \

SLV2_WRAP_COLL(PluginClasses, PluginClass, plugin_classes);
SLV2_WRAP_COLL(ScalePoints, ScalePoint, scale_points);
SLV2_WRAP_COLL(Values, Value, values);

struct Plugins {
	inline Plugins(SLV2Plugins c_obj) : me(c_obj) {}
	inline operator SLV2Plugins() const { return me; }

	SLV2Plugins me;
};

struct World {
	inline World() : me(slv2_world_new()) {}
	inline ~World() { /*slv2_world_free(me);*/ }

	inline SLV2Value new_uri(const char* uri) {
		return slv2_value_new_uri(me, uri);
	}
	inline SLV2Value new_string(const char* str) {
		return slv2_value_new_string(me, str);
	}
	inline SLV2Value new_int(int val) {
		return slv2_value_new_int(me, val);
	}
	inline SLV2Value new_float(float val) {
		return slv2_value_new_float(me, val);
	}
	inline SLV2Value new_bool(bool val) {
		return slv2_value_new_bool(me, val);
	}

	SLV2_WRAP2_VOID(world, set_option, const char*, uri, SLV2Value, value);
	SLV2_WRAP0_VOID(world, free);
	SLV2_WRAP0_VOID(world, load_all);
	SLV2_WRAP1_VOID(world, load_bundle, SLV2Value, bundle_uri);
	SLV2_WRAP0(SLV2PluginClass, world, get_plugin_class);
	SLV2_WRAP0(SLV2PluginClasses, world, get_plugin_classes);
	SLV2_WRAP0(Plugins, world, get_all_plugins);
	//SLV2_WRAP1(Plugin, world, get_plugin_by_uri_string, const char*, uri);

	SLV2World me;
};

struct Port {
	inline Port(SLV2Plugin p, SLV2Port c_obj) : parent(p), me(c_obj) {}
	inline operator SLV2Port() const { return me; }

#define SLV2_PORT_WRAP0(RT, name) \
	inline RT name () { return slv2_port_ ## name (parent, me); }

#define SLV2_PORT_WRAP1(RT, name, T1, a1) \
	inline RT name (T1 a1) { return slv2_port_ ## name (parent, me, a1); }

	SLV2_PORT_WRAP1(SLV2Values, get_value, SLV2Value, predicate);
	SLV2_PORT_WRAP1(SLV2Values, get_value_by_qname, const char*, predicate);
	SLV2_PORT_WRAP0(SLV2Values, get_properties)
	SLV2_PORT_WRAP1(bool, has_property, SLV2Value, property_uri);
	SLV2_PORT_WRAP1(bool, supports_event, SLV2Value, event_uri);
	SLV2_PORT_WRAP0(SLV2Value,  get_symbol);
	SLV2_PORT_WRAP0(SLV2Value,  get_name);
	SLV2_PORT_WRAP0(SLV2Values, get_classes);
	SLV2_PORT_WRAP1(bool, is_a, SLV2Value, port_class);
	SLV2_PORT_WRAP0(SLV2ScalePoints, get_scale_points);

	// TODO: get_range (output parameters)

	SLV2Plugin parent;
	SLV2Port   me;
};

struct Plugin {
	inline Plugin(SLV2Plugin c_obj) : me(c_obj) {}
	inline operator SLV2Plugin() const { return me; }

	SLV2_WRAP0(bool,        plugin, verify);
	SLV2_WRAP0(Value,       plugin, get_uri);
	SLV2_WRAP0(Value,       plugin, get_bundle_uri);
	SLV2_WRAP0(Values,      plugin, get_data_uris);
	SLV2_WRAP0(Value,       plugin, get_library_uri);
	SLV2_WRAP0(Value,       plugin, get_name);
	SLV2_WRAP0(PluginClass, plugin, get_class);
	SLV2_WRAP1(Values,      plugin, get_value, Value, pred);
	SLV2_WRAP1(Values,      plugin, get_value_by_qname, const char*, predicate);
	SLV2_WRAP2(Values,      plugin, get_value_for_subject, Value, subject,
	                                                       Value, predicate);
	SLV2_WRAP1(bool,        plugin, has_feature, Value, feature_uri);
	SLV2_WRAP0(Values,      plugin, get_supported_features);
	SLV2_WRAP0(Values,      plugin, get_required_features);
	SLV2_WRAP0(Values,      plugin, get_optional_features);
	SLV2_WRAP0(unsigned,    plugin, get_num_ports);
	SLV2_WRAP0(bool,        plugin, has_latency);
	SLV2_WRAP0(unsigned,    plugin, get_latency_port_index);
	SLV2_WRAP0(Value,       plugin, get_author_name);
	SLV2_WRAP0(Value,       plugin, get_author_email);
	SLV2_WRAP0(Value,       plugin, get_author_homepage);

	inline Port get_port_by_index(unsigned index) {
		return Port(me, slv2_plugin_get_port_by_index(me, index));
	}

	inline Port get_port_by_symbol(SLV2Value symbol) {
		return Port(me, slv2_plugin_get_port_by_symbol(me, symbol));
	}

	inline void get_port_ranges_float(float* min_values,
	                                  float* max_values,
	                                  float* def_values) {
		return slv2_plugin_get_port_ranges_float(
			me, min_values, max_values, def_values);
	}

	inline unsigned get_num_ports_of_class(SLV2Value class_1,
	                                       SLV2Value class_2) {
		// TODO: varargs
		return slv2_plugin_get_num_ports_of_class(me, class_1, class_2, NULL);
	}

	SLV2Plugin me;
};

struct Instance {
	inline Instance(Plugin plugin, double sample_rate) {
		// TODO: features
		me = slv2_plugin_instantiate(plugin, sample_rate, NULL);
	}

	inline operator SLV2Instance() const { return me; }

	SLV2_WRAP2_VOID(instance, connect_port,
	                unsigned, port_index,
	                void*,    data_location);

	SLV2_WRAP0_VOID(instance, activate);
	SLV2_WRAP1_VOID(instance, run, unsigned, sample_count);
	SLV2_WRAP0_VOID(instance, deactivate);

	// TODO: get_extension_data

	inline const LV2_Descriptor* get_descriptor() {
		return slv2_instance_get_descriptor(me);
	}

	SLV2Instance me;
};

} /* namespace SLV2 */

#endif /* SLV2_SLV2MM_HPP */
