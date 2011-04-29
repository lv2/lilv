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

#ifndef LILV_LILVMM_HPP
#define LILV_LILVMM_HPP

#include "lilv/lilv.h"

namespace Lilv {

const char* uri_to_path(const char* uri) { return lilv_uri_to_path(uri); }

#define LILV_WRAP0(RT, prefix, name) \
	inline RT name () { return lilv_ ## prefix ## _ ## name (me); }

#define LILV_WRAP0_VOID(prefix, name) \
	inline void name () { lilv_ ## prefix ## _ ## name (me); }

#define LILV_WRAP1(RT, prefix, name, T1, a1) \
	inline RT name (T1 a1) { return lilv_ ## prefix ## _ ## name (me, a1); }

#define LILV_WRAP1_VOID(prefix, name, T1, a1) \
	inline void name (T1 a1) { lilv_ ## prefix ## _ ## name (me, a1); }

#define LILV_WRAP2(RT, prefix, name, T1, a1, T2, a2) \
	inline RT name (T1 a1, T2 a2) { return lilv_ ## prefix ## _ ## name (me, a1, a2); }

#define LILV_WRAP2_VOID(prefix, name, T1, a1, T2, a2) \
	inline void name (T1 a1, T2 a2) { lilv_ ## prefix ## _ ## name (me, a1, a2); }

#ifndef SWIG
#define LILV_WRAP_CONVERSION(CT) \
	inline operator CT*() const { return me; }
#else
#define LILV_WRAP_CONVERSION(CT)
#endif

struct Value {
	inline Value(const LilvValue* value) : me(lilv_value_duplicate(value)) {}
	inline Value(const Value& copy)      : me(lilv_value_duplicate(copy.me)) {}

	inline ~Value() { lilv_value_free(me); }

	inline bool equals(const Value& other) const {
		return lilv_value_equals(me, other.me);
	}

	inline bool operator==(const Value& other) const { return equals(other); }

	LILV_WRAP_CONVERSION(LilvValue);

	LILV_WRAP0(char*,       value, get_turtle_token);
	LILV_WRAP0(bool,        value, is_uri);
	LILV_WRAP0(const char*, value, as_uri);
	LILV_WRAP0(bool,        value, is_blank);
	LILV_WRAP0(const char*, value, as_blank);
	LILV_WRAP0(bool,        value, is_literal);
	LILV_WRAP0(bool,        value, is_string);
	LILV_WRAP0(const char*, value, as_string);
	LILV_WRAP0(bool,        value, is_float);
	LILV_WRAP0(float,       value, as_float);
	LILV_WRAP0(bool,        value, is_int);
	LILV_WRAP0(int,         value, as_int);
	LILV_WRAP0(bool,        value, is_bool);
	LILV_WRAP0(bool,        value, as_bool);

	LilvValue* me;
};

struct ScalePoint {
	inline ScalePoint(const LilvScalePoint* c_obj) : me(c_obj) {}
	LILV_WRAP_CONVERSION(const LilvScalePoint);

	LILV_WRAP0(const LilvValue*, scale_point, get_label);
	LILV_WRAP0(const LilvValue*, scale_point, get_value);

	const LilvScalePoint* me;
};

struct PluginClass {
	inline PluginClass(const LilvPluginClass* c_obj) : me(c_obj) {}
	LILV_WRAP_CONVERSION(const LilvPluginClass);

	LILV_WRAP0(Value, plugin_class, get_parent_uri);
	LILV_WRAP0(Value, plugin_class, get_uri);
	LILV_WRAP0(Value, plugin_class, get_label);

	inline LilvPluginClasses* get_children() {
		return lilv_plugin_class_get_children(me);
	}

	const LilvPluginClass* me;
};

#define LILV_WRAP_COLL(CT, ET, prefix) \
	struct CT { \
		inline CT(const Lilv ## CT* c_obj) : me(c_obj) {} \
		LILV_WRAP_CONVERSION(const Lilv ## CT); \
		LILV_WRAP0(unsigned, prefix, size); \
		LILV_WRAP1(const ET, prefix, get, LilvIter*, i); \
		const Lilv ## CT* me; \
	}; \

LILV_WRAP_COLL(PluginClasses, PluginClass, plugin_classes);
LILV_WRAP_COLL(ScalePoints, ScalePoint, scale_points);
LILV_WRAP_COLL(Values, Value, values);

struct Plugins {
	inline Plugins(const LilvPlugins* c_obj) : me(c_obj) {}
	LILV_WRAP_CONVERSION(const LilvPlugins);

	const LilvPlugins* me;
};

struct World {
	inline World() : me(lilv_world_new()) {}
	inline ~World() { /*lilv_world_free(me);*/ }

	inline LilvValue* new_uri(const char* uri) {
		return lilv_new_uri(me, uri);
	}
	inline LilvValue* new_string(const char* str) {
		return lilv_new_string(me, str);
	}
	inline LilvValue* new_int(int val) {
		return lilv_new_int(me, val);
	}
	inline LilvValue* new_float(float val) {
		return lilv_new_float(me, val);
	}
	inline LilvValue* new_bool(bool val) {
		return lilv_new_bool(me, val);
	}

	LILV_WRAP2_VOID(world, set_option, const char*, uri, LilvValue*, value);
	LILV_WRAP0_VOID(world, free);
	LILV_WRAP0_VOID(world, load_all);
	LILV_WRAP1_VOID(world, load_bundle, LilvValue*, bundle_uri);
	LILV_WRAP0(const LilvPluginClass*, world, get_plugin_class);
	LILV_WRAP0(const LilvPluginClasses*, world, get_plugin_classes);
	LILV_WRAP0(Plugins, world, get_all_plugins);
	//LILV_WRAP1(Plugin, world, get_plugin_by_uri_string, const char*, uri);

	LilvWorld* me;
};

struct Port {
	inline Port(const LilvPlugin* p, const LilvPort* c_obj) : parent(p), me(c_obj) {}
	LILV_WRAP_CONVERSION(const LilvPort);

#define LILV_PORT_WRAP0(RT, name) \
	inline RT name () { return lilv_port_ ## name (parent, me); }

#define LILV_PORT_WRAP1(RT, name, T1, a1) \
	inline RT name (T1 a1) { return lilv_port_ ## name (parent, me, a1); }

	LILV_PORT_WRAP1(LilvValues*, get_value, LilvValue*, predicate);
	LILV_PORT_WRAP0(LilvValues*, get_properties)
	LILV_PORT_WRAP1(bool, has_property, LilvValue*, property_uri);
	LILV_PORT_WRAP1(bool, supports_event, LilvValue*, event_uri);
	LILV_PORT_WRAP0(const LilvValue*,  get_symbol);
	LILV_PORT_WRAP0(LilvValue*,  get_name);
	LILV_PORT_WRAP0(const LilvValues*, get_classes);
	LILV_PORT_WRAP1(bool, is_a, LilvValue*, port_class);
	LILV_PORT_WRAP0(LilvScalePoints*, get_scale_points);

	// TODO: get_range (output parameters)

	const LilvPlugin* parent;
	const LilvPort*   me;
};

struct Plugin {
	inline Plugin(const LilvPlugin* c_obj) : me(c_obj) {}
	LILV_WRAP_CONVERSION(const LilvPlugin);

	LILV_WRAP0(bool,        plugin, verify);
	LILV_WRAP0(Value,       plugin, get_uri);
	LILV_WRAP0(Value,       plugin, get_bundle_uri);
	LILV_WRAP0(Values,      plugin, get_data_uris);
	LILV_WRAP0(Value,       plugin, get_library_uri);
	LILV_WRAP0(Value,       plugin, get_name);
	LILV_WRAP0(PluginClass, plugin, get_class);
	LILV_WRAP1(Values,      plugin, get_value, Value, pred);
	LILV_WRAP2(Values,      plugin, get_value_for_subject, Value, subject,
	                                                       Value, predicate);
	LILV_WRAP1(bool,        plugin, has_feature, Value, feature_uri);
	LILV_WRAP0(Values,      plugin, get_supported_features);
	LILV_WRAP0(Values,      plugin, get_required_features);
	LILV_WRAP0(Values,      plugin, get_optional_features);
	LILV_WRAP0(unsigned,    plugin, get_num_ports);
	LILV_WRAP0(bool,        plugin, has_latency);
	LILV_WRAP0(unsigned,    plugin, get_latency_port_index);
	LILV_WRAP0(Value,       plugin, get_author_name);
	LILV_WRAP0(Value,       plugin, get_author_email);
	LILV_WRAP0(Value,       plugin, get_author_homepage);

	inline Port get_port_by_index(unsigned index) {
		return Port(me, lilv_plugin_get_port_by_index(me, index));
	}

	inline Port get_port_by_symbol(LilvValue* symbol) {
		return Port(me, lilv_plugin_get_port_by_symbol(me, symbol));
	}

	inline void get_port_ranges_float(float* min_values,
	                                  float* max_values,
	                                  float* def_values) {
		return lilv_plugin_get_port_ranges_float(
			me, min_values, max_values, def_values);
	}

	inline unsigned get_num_ports_of_class(LilvValue* class_1,
	                                       LilvValue* class_2) {
		// TODO: varargs
		return lilv_plugin_get_num_ports_of_class(me, class_1, class_2, NULL);
	}

	const LilvPlugin* me;
};

struct Instance {
	inline Instance(Plugin plugin, double sample_rate) {
		// TODO: features
		me = lilv_plugin_instantiate(plugin, sample_rate, NULL);
	}

	LILV_WRAP_CONVERSION(LilvInstance);

	LILV_WRAP2_VOID(instance, connect_port,
	                unsigned, port_index,
	                void*,    data_location);

	LILV_WRAP0_VOID(instance, activate);
	LILV_WRAP1_VOID(instance, run, unsigned, sample_count);
	LILV_WRAP0_VOID(instance, deactivate);

	// TODO: get_extension_data

	inline const LV2_Descriptor* get_descriptor() {
		return lilv_instance_get_descriptor(me);
	}

	LilvInstance* me;
};

} /* namespace Lilv */

#endif /* LILV_LILVMM_HPP */
