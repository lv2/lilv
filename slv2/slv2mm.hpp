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

#ifndef SLV2_SLV2MM_HPP__
#define SLV2_SLV2MM_HPP__

#include "slv2/slv2.h"

namespace SLV2 {

const char* uri_to_path(const char* uri) { return slv2_uri_to_path(uri); }

class Value {
public:
	inline Value(SLV2Value value)   : me(value) {}
	inline Value(const Value& copy) : me(slv2_value_duplicate(copy.me)) {}

	inline ~Value() { slv2_value_free(me); }

	inline bool equals(const Value& other) const {
		return slv2_value_equals(me, other.me);
	}

	inline bool operator==(const Value& other) const { return equals(other); }

	inline operator SLV2Value() const { return me; }

#define VAL_WRAP0(RetType, name) \
	inline RetType name () { return slv2_value_ ## name (me); }

	VAL_WRAP0(char*,       get_turtle_token);
	VAL_WRAP0(bool,        is_uri);
	VAL_WRAP0(const char*, as_uri);
	VAL_WRAP0(bool,        is_blank);
	VAL_WRAP0(const char*, as_blank);
	VAL_WRAP0(bool,        is_literal);
	VAL_WRAP0(bool,        is_string);
	VAL_WRAP0(const char*, as_string);
	VAL_WRAP0(bool,        is_float);
	VAL_WRAP0(float,       as_float);
	VAL_WRAP0(bool,        is_int);
	VAL_WRAP0(int,         as_int);
	VAL_WRAP0(bool,        is_bool);
	VAL_WRAP0(bool,        as_bool);

	SLV2Value me;
};

class Plugins {
public:
	inline Plugins(SLV2Plugins c_obj) : me(c_obj) {}

	inline operator SLV2Plugins() const { return me; }

	SLV2Plugins me;
};

class World {
public:
	inline World() : me(slv2_world_new()) {}

	inline ~World() { /*slv2_world_free(me);*/ }

#define WORLD_WRAP_VOID0(name) \
	inline void name () { slv2_world_ ## name (me); }

#define WORLD_WRAP0(RetType, name) \
	inline RetType name () { return slv2_world_ ## name (me); }

#define WORLD_WRAP1(RetType, name, T1, a1) \
	inline RetType name (T1 a1) { return slv2_world_ ## name (me, a1); }

#define WORLD_WRAP2(RetType, name, T1, a1, T2, a2) \
	inline RetType name (T1 a1, T2 a2) { return slv2_world_ ## name (me, a1, a2); }
	
	WORLD_WRAP2(void, set_option, const char*, uri, SLV2Value, value);
	WORLD_WRAP_VOID0(free);
	WORLD_WRAP_VOID0(load_all);
	WORLD_WRAP1(void, load_bundle, SLV2Value, bundle_uri);
	WORLD_WRAP0(SLV2PluginClass, get_plugin_class);
	WORLD_WRAP0(SLV2PluginClasses, get_plugin_classes);
	WORLD_WRAP0(Plugins, get_all_plugins);
	// TODO: get_plugins_by_filter (takes a function parameter)

	SLV2World me;
};

class Port {
public:
	inline Port(SLV2Port c_obj) : me(c_obj) {}

	inline operator SLV2Port() const { return me; }

	SLV2Port me;
};

class Plugin {
public:
	inline Plugin(SLV2Plugin c_obj) : me(c_obj) {}

	inline operator SLV2Plugin() const { return me; }

#define PLUGIN_WRAP0(RetType, name) \
	inline RetType name () { return slv2_plugin_ ## name (me); }

#define PLUGIN_WRAP1(RetType, name, T1, a1)	  \
	inline RetType name (T1 a1) { return slv2_plugin_ ## name (me, a1); }

#define PLUGIN_WRAP2(RetType, name, T1, a1, T2, a2)	  \
	inline RetType name (T1 a1, T2 a2) { return slv2_plugin_ ## name (me, a1, a2); }

	PLUGIN_WRAP0(bool,  verify);
	PLUGIN_WRAP0(Value, get_uri);
	PLUGIN_WRAP0(Value, get_bundle_uri);
	//PLUGIN_WRAP0(Values, get_data_uris);
	PLUGIN_WRAP0(Value, get_library_uri);
	PLUGIN_WRAP0(Value, get_name);
	//PLUGIN_WRAP0(PluginClass, get_class);
	//PLUGIN_WRAP1(Values, get_value, Value, predicate);
	//PLUGIN_WRAP1(Values, get_value_by_qname, const char*, predicate);
	//PLUGIN_WRAP2(Values, get_value_for_subject, Value, subject, Value, predicate);
	PLUGIN_WRAP1(bool, has_feature, Value, feature_uri);
	//PLUGIN_WRAP0(Values, get_supported_features);
	//PLUGIN_WRAP0(Values, get_required_features);
	//PLUGIN_WRAP0(Values, get_optional_features);
	PLUGIN_WRAP0(uint32_t, get_num_ports);
	// slv2_plugin_get_port_ranges_float
	// slv2_plugin_get_num_ports_of_class
	PLUGIN_WRAP0(bool, has_latency);
	PLUGIN_WRAP0(uint32_t, get_latency_port_index);
	PLUGIN_WRAP1(Port, get_port_by_index, uint32_t, index);
	PLUGIN_WRAP1(Port, get_port_by_symbol, SLV2Value, symbol);
	PLUGIN_WRAP0(Value, get_author_name);
	PLUGIN_WRAP0(Value, get_author_email);
	PLUGIN_WRAP0(Value, get_author_homepage);

	SLV2Plugin me;
};
	
} /* namespace SLV2 */

#endif /* SLV2_SLV2MM_HPP__ */
