/* SLV2 Tests
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
 * Copyright (C) 2008 Krzysztof Foltman
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

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <librdf.h>
#include <sys/stat.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include "slv2/slv2.h"

#define TEST_PATH_MAX 1024

static char bundle_dir_name[TEST_PATH_MAX];
static char bundle_dir_uri[TEST_PATH_MAX];
static char manifest_name[TEST_PATH_MAX];
static char content_name[TEST_PATH_MAX];

static SLV2World world;

int test_count  = 0;
int error_count = 0;

void
delete_bundle()
{
	unlink(content_name);
	unlink(manifest_name);
	rmdir(bundle_dir_name);
}

void
init_tests()
{
	strncpy(bundle_dir_name, getenv("HOME"), 900);
	strcat(bundle_dir_name, "/.lv2/slv2-test.lv2");
	sprintf(bundle_dir_uri, "file://%s/", bundle_dir_name);
	sprintf(manifest_name, "%s/manifest.ttl", bundle_dir_name);
	sprintf(content_name, "%s/plugin.ttl", bundle_dir_name);

	delete_bundle();
}

void
fatal_error(const char *err, const char *arg)
{
	/* TODO: possibly change to vfprintf later */
	fprintf(stderr, err, arg);
	/* IMHO, the bundle should be left in place after an error, for possible investigation */
	/* delete_bundle(); */
	exit(1);
}

void
write_file(const char *name, const char *content)
{
	FILE* f = fopen(name, "w");
	size_t len = strlen(content);
	if (fwrite(content, 1, len, f) != len)
		fatal_error("Cannot write file %s\n", name);
	fclose(f);
}

int
init_world()
{
	world = slv2_world_new();
	return world != NULL;
}

int
load_all_bundles()
{
	if (!init_world())
		return 0;
	slv2_world_load_all(world);
	return 1;
}

int
load_bundle()
{
	SLV2Value uri;
	if (!init_world())
		return 0;
	uri = slv2_value_new_uri(world, bundle_dir_uri);
	slv2_world_load_bundle(world, uri);
	slv2_value_free(uri);
	return 1;
}

void
create_bundle(char *manifest, char *content)
{
	if (mkdir(bundle_dir_name, 0700))
		fatal_error("Cannot create directory %s\n", bundle_dir_name);
	write_file(manifest_name, manifest);
	write_file(content_name, content);
}

int
start_bundle(char *manifest, char *content, int load_all)
{
	create_bundle(manifest, content);
	if (load_all)
		return load_all_bundles();
	else
		return load_bundle();
}

void
unload_bundle()
{
	if (world)
		slv2_world_free(world);
	world = NULL;
}

void
cleanup()
{
	delete_bundle();
}

/*****************************************************************************/

#define TEST_CASE(name) { #name, test_##name }
#define TEST_ASSERT(check) do {\
	test_count++;\
	if (!(check)) {\
		error_count++;\
		fprintf(stderr, "Failure at slv2_test.c:%d: %s\n", __LINE__, #check);\
	}\
} while (0);

typedef int (*TestFunc)();

struct TestCase {
	const char *title;
	TestFunc func;
};

#define PREFIX_LINE "@prefix : <http://example.org/> .\n"
#define PREFIX_LV2 "@prefix lv2: <http://lv2plug.in/ns/lv2core#> .\n"
#define PREFIX_LV2EV "@prefix lv2ev: <http://lv2plug.in/ns/ext/event#> . \n"
#define PREFIX_LV2UI "@prefix lv2ui: <http://lv2plug.in/ns/extensions/ui#> .\n"
#define PREFIX_RDFS "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
#define PREFIX_FOAF "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
#define PREFIX_DOAP "@prefix doap: <http://usefulinc.com/ns/doap#> .\n"

#define MANIFEST_PREFIXES PREFIX_LINE PREFIX_LV2 PREFIX_RDFS
#define BUNDLE_PREFIXES PREFIX_LINE PREFIX_LV2 PREFIX_RDFS PREFIX_FOAF PREFIX_DOAP
#define PLUGIN_NAME(name) "doap:name \"" name "\""
#define LICENSE_GPL "doap:license <http://usefulinc.com/doap/licenses/gpl>"

static char *uris_plugin = "http://example.org/plug";
static SLV2Value plugin_uri_value, plugin2_uri_value;

/*****************************************************************************/

void
init_uris()
{
	plugin_uri_value = slv2_value_new_uri(world, uris_plugin);
	plugin2_uri_value = slv2_value_new_uri(world, "http://example.org/foobar");
	TEST_ASSERT(plugin_uri_value);
	TEST_ASSERT(plugin2_uri_value);
}

void
cleanup_uris()
{
	slv2_value_free(plugin2_uri_value);
	slv2_value_free(plugin_uri_value);
	plugin2_uri_value = NULL;
	plugin_uri_value = NULL;
}

/*****************************************************************************/

int
test_utils()
{
	TEST_ASSERT(!strcmp(slv2_uri_to_path("file:///tmp/blah"), "/tmp/blah"));
	TEST_ASSERT(!slv2_uri_to_path("file:/example.org/blah"));
	TEST_ASSERT(!slv2_uri_to_path("http://example.org/blah"));
	return 1;
}

/*****************************************************************************/

int
test_value()
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin ; a lv2:CompressorPlugin ; "
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
			"lv2:port [ "
			"  a lv2:ControlPort ; a lv2:InputPort ; "
			"  lv2:index 0 ; lv2:symbol \"foo\" ; lv2:name \"Foo\" ; "
			"] .",
			1))
		return 0;

	init_uris();

	SLV2Value uval = slv2_value_new_uri(world, "http://example.org");
	SLV2Value sval = slv2_value_new_string(world, "Foo");
	SLV2Value ival = slv2_value_new_int(world, 42);
	SLV2Value fval = slv2_value_new_float(world, 1.6180);

	TEST_ASSERT(slv2_value_is_uri(uval));
	TEST_ASSERT(slv2_value_is_string(sval));
	TEST_ASSERT(slv2_value_is_int(ival));
	TEST_ASSERT(slv2_value_is_float(fval));
	
	TEST_ASSERT(!slv2_value_is_literal(uval));
	TEST_ASSERT(slv2_value_is_literal(sval));
	TEST_ASSERT(slv2_value_is_literal(ival));
	TEST_ASSERT(slv2_value_is_literal(fval));

	TEST_ASSERT(!strcmp(slv2_value_as_uri(uval), "http://example.org"));
	TEST_ASSERT(!strcmp(slv2_value_as_string(sval), "Foo"));
	TEST_ASSERT(slv2_value_as_int(ival) == 42);
	TEST_ASSERT(fabs(slv2_value_as_float(fval) - 1.6180) < FLT_EPSILON);

	TEST_ASSERT(!strcmp(slv2_value_get_turtle_token(uval), "<http://example.org>"));
	TEST_ASSERT(!strcmp(slv2_value_get_turtle_token(sval), "Foo"));
	TEST_ASSERT(!strcmp(slv2_value_get_turtle_token(ival), "42"));
	TEST_ASSERT(!strncmp(slv2_value_get_turtle_token(fval), "1.6180", 6));
	
	SLV2Value uval_e = slv2_value_new_uri(world, "http://example.org");
	SLV2Value sval_e = slv2_value_new_string(world, "Foo");
	SLV2Value ival_e = slv2_value_new_int(world, 42);
	SLV2Value fval_e = slv2_value_new_float(world, 1.6180);
	SLV2Value uval_ne = slv2_value_new_uri(world, "http://no-example.org");
	SLV2Value sval_ne = slv2_value_new_string(world, "Bar");
	SLV2Value ival_ne = slv2_value_new_int(world, 24);
	SLV2Value fval_ne = slv2_value_new_float(world, 3.14159);

	TEST_ASSERT(slv2_value_equals(uval, uval_e));
	TEST_ASSERT(slv2_value_equals(sval, sval_e));
	TEST_ASSERT(slv2_value_equals(ival, ival_e));
	TEST_ASSERT(slv2_value_equals(fval, fval_e));
	
	TEST_ASSERT(!slv2_value_equals(uval, uval_ne));
	TEST_ASSERT(!slv2_value_equals(sval, sval_ne));
	TEST_ASSERT(!slv2_value_equals(ival, ival_ne));
	TEST_ASSERT(!slv2_value_equals(fval, fval_ne));
	
	TEST_ASSERT(!slv2_value_equals(uval, sval));
	TEST_ASSERT(!slv2_value_equals(sval, ival));
	TEST_ASSERT(!slv2_value_equals(ival, fval));
	
	SLV2Value uval_dup = slv2_value_duplicate(uval);
	TEST_ASSERT(slv2_value_equals(uval, uval_dup));

	SLV2Value ifval = slv2_value_new_float(world, 42.0);
	TEST_ASSERT(!slv2_value_equals(ival, ifval));
	
	SLV2Value nil = NULL;
	TEST_ASSERT(!slv2_value_equals(uval, nil));
	TEST_ASSERT(!slv2_value_equals(nil, uval));
	TEST_ASSERT(slv2_value_equals(nil, nil));

	SLV2Value nil2 = slv2_value_duplicate(nil);
	TEST_ASSERT(slv2_value_equals(nil, nil2));

	slv2_value_free(uval);
	slv2_value_free(sval);
	slv2_value_free(ival);
	slv2_value_free(fval);
	slv2_value_free(uval_e);
	slv2_value_free(sval_e);
	slv2_value_free(ival_e);
	slv2_value_free(fval_e);
	slv2_value_free(uval_ne);
	slv2_value_free(sval_ne);
	slv2_value_free(ival_ne);
	slv2_value_free(fval_ne);
	slv2_value_free(uval_dup);

	cleanup_uris();
	return 1;
}

/*****************************************************************************/

int
test_values()
{
	init_world();
	SLV2Value v0 = slv2_value_new_uri(world, "http://example.org/");
	SLV2Values vs1 = slv2_values_new();
	TEST_ASSERT(vs1);
	TEST_ASSERT(!slv2_values_size(vs1));
	TEST_ASSERT(!slv2_values_contains(vs1, v0));
	slv2_values_free(vs1);
	slv2_value_free(v0);
	return 1;
}

/*****************************************************************************/

static int discovery_plugin_found = 0;

static bool
discovery_plugin_filter_all(SLV2Plugin plugin)
{
	return true;
}

static bool
discovery_plugin_filter_none(SLV2Plugin plugin)
{
	return false;
}

static bool
discovery_plugin_filter_ours(SLV2Plugin plugin)
{
	return slv2_value_equals(slv2_plugin_get_uri(plugin), plugin_uri_value);
}

static bool
discovery_plugin_filter_fake(SLV2Plugin plugin)
{
	return slv2_value_equals(slv2_plugin_get_uri(plugin), plugin2_uri_value);
}

static void
discovery_verify_plugin(SLV2Plugin plugin)
{
	SLV2Value value = slv2_plugin_get_uri(plugin);
	if (slv2_value_equals(value, plugin_uri_value)) {
		SLV2Value lib_uri = NULL;
		TEST_ASSERT(!slv2_value_equals(value, plugin2_uri_value));
		discovery_plugin_found = 1;
		lib_uri = slv2_plugin_get_library_uri(plugin);
		TEST_ASSERT(lib_uri);
		TEST_ASSERT(slv2_value_is_uri(lib_uri));
		TEST_ASSERT(slv2_value_as_uri(lib_uri));
		TEST_ASSERT(strstr(slv2_value_as_uri(lib_uri), "foo.so"));
		/* this is already being tested as ticket291, but the discovery and ticket291
		 * may diverge at some point, so I'm duplicating it here */
		TEST_ASSERT(slv2_plugin_verify(plugin));
	}
}

int
test_discovery_variant(int load_all)
{
	SLV2Plugins plugins;

	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin ;"
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
			"lv2:port [ a lv2:ControlPort ; a lv2:InputPort ;"
			" lv2:index 0 ; lv2:symbol \"foo\" ; lv2:name \"bar\" ; ] .",
			load_all))
		return 0;

	init_uris();

	/* lookup 1: all plugins (get_all_plugins)
	 * lookup 2: all plugins (get_plugins_by_filter, always true)
	 * lookup 3: no plugins (get_plugins_by_filter, always false)
	 * lookup 4: only example plugin (get_plugins_by_filter)
	 * lookup 5: no plugins (get_plugins_by_filter, non-existing plugin)
	 */
	for (int lookup = 1; lookup <= 5; lookup++) {
		//printf("Lookup variant %d\n", lookup);
		int expect_found = 0;
		switch (lookup) {
		case 1:
			plugins = slv2_world_get_all_plugins(world);
			TEST_ASSERT(slv2_plugins_size(plugins) > 0);
			expect_found = 1;
			break;
		case 2:
			plugins = slv2_world_get_plugins_by_filter(world,
					discovery_plugin_filter_all);
			TEST_ASSERT(slv2_plugins_size(plugins) > 0);
			expect_found = 1;
			break;
		case 3:
			plugins = slv2_world_get_plugins_by_filter(world,
					discovery_plugin_filter_none);
			TEST_ASSERT(slv2_plugins_size(plugins) == 0);
			break;
		case 4:
			plugins = slv2_world_get_plugins_by_filter(world,
					discovery_plugin_filter_ours);
			TEST_ASSERT(slv2_plugins_size(plugins) == 1);
			expect_found = 1;
			break;
		case 5:
			plugins = slv2_world_get_plugins_by_filter(world,
					discovery_plugin_filter_fake);
			TEST_ASSERT(slv2_plugins_size(plugins) == 0);
			break;
		}

		SLV2Plugin explug = slv2_plugins_get_by_uri(plugins, plugin_uri_value);
		TEST_ASSERT((explug != NULL) == expect_found);
		SLV2Plugin explug2 = slv2_plugins_get_by_uri(plugins, plugin2_uri_value);
		TEST_ASSERT(explug2 == NULL);

		if (explug && expect_found) {
			TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_plugin_get_name(explug)),
					"Test plugin"));
		}

		discovery_plugin_found = 0;
		for (size_t i = 0; i < slv2_plugins_size(plugins); i++)
			discovery_verify_plugin(slv2_plugins_get_at(plugins, i));

		TEST_ASSERT(discovery_plugin_found == expect_found);
		slv2_plugins_free(world, plugins);
		plugins = NULL;
	}

	TEST_ASSERT(slv2_plugins_get_at(plugins, (unsigned)INT_MAX + 1) == NULL);

	cleanup_uris();

	return 1;
}

int
test_discovery_load_bundle()
{
	return test_discovery_variant(0);
}

int
test_discovery_load_all()
{
	return test_discovery_variant(1);
}

/*****************************************************************************/

int
test_verify()
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin ; "
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
			"lv2:port [ a lv2:ControlPort ; a lv2:InputPort ;"
			" lv2:index 0 ; lv2:symbol \"foo\" ; lv2:name \"bar\" ] .",
			1))
		return 0;

	init_uris();
	SLV2Plugins plugins = slv2_world_get_all_plugins(world);
	SLV2Plugin explug = slv2_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(explug);
	TEST_ASSERT(slv2_plugin_verify(explug));
	slv2_plugins_free(world, plugins);
	cleanup_uris();
	return 1;
}

/*****************************************************************************/

int
test_no_verify()
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin . ",
			1))
		return 0;

	init_uris();
	SLV2Plugins plugins = slv2_world_get_all_plugins(world);
	SLV2Plugin explug = slv2_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(explug);
	TEST_ASSERT(!slv2_plugin_verify(explug));
	slv2_plugins_free(world, plugins);
	cleanup_uris();
	return 1;
}

/*****************************************************************************/

int
test_classes()
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin ; a lv2:CompressorPlugin ; "
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
			"lv2:port [ "
			"  a lv2:ControlPort ; a lv2:InputPort ; "
			"  lv2:index 0 ; lv2:symbol \"foo\" ; lv2:name \"Foo\" ; "
			"] .",
			1))
		return 0;

	init_uris();
	SLV2PluginClass plugin = slv2_world_get_plugin_class(world);
	SLV2PluginClasses classes = slv2_world_get_plugin_classes(world);
	SLV2PluginClasses children = slv2_plugin_class_get_children(plugin);
	
	TEST_ASSERT(slv2_plugin_class_get_parent_uri(plugin) == NULL);
	TEST_ASSERT(slv2_plugin_classes_size(classes) > slv2_plugin_classes_size(children))
	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_plugin_class_get_label(plugin)), "Plugin"));
	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_plugin_class_get_uri(plugin)),
			"http://lv2plug.in/ns/lv2core#Plugin"));

	for (unsigned i = 0; i < slv2_plugin_classes_size(children); ++i) {
		TEST_ASSERT(slv2_value_equals(
				slv2_plugin_class_get_parent_uri(slv2_plugin_classes_get_at(children, i)),
				slv2_plugin_class_get_uri(plugin)));
	}

	SLV2Value some_uri = slv2_value_new_uri(world, "http://example.org/whatever");
	TEST_ASSERT(slv2_plugin_classes_get_by_uri(classes, some_uri) == NULL);
	slv2_value_free(some_uri);
	
	TEST_ASSERT(slv2_plugin_classes_get_at(classes, (unsigned)INT_MAX + 1) == NULL);

	cleanup_uris();
	return 1;
}


/*****************************************************************************/

int
test_plugin()
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin ; a lv2:CompressorPlugin ; "
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
    		"lv2:optionalFeature lv2:hardRtCapable ; "
		    "lv2:requiredFeature <http://lv2plug.in/ns/ext/event> ; "
			"doap:maintainer [ foaf:name \"David Robillard\" ; "
			"  foaf:homepage <http://drobilla.net> ; foaf:mbox <mailto:dave@drobilla.net> ] ; "
			"lv2:port [ "
			"  a lv2:ControlPort ; a lv2:InputPort ; "
			"  lv2:index 0 ; lv2:symbol \"foo\" ; lv2:name \"bar\" ; "
			"  lv2:minimum -1.0 ; lv2:maximum 1.0 ; lv2:default 0.5 "
			"] , [ "
			"  a lv2:ControlPort ; a lv2:InputPort ; "
			"  lv2:index 1 ; lv2:symbol \"bar\" ; lv2:name \"Baz\" ; "
			"  lv2:minimum -2.0 ; lv2:maximum 2.0 ; lv2:default 1.0 "
			"] , [ "
			"  a lv2:ControlPort ; a lv2:OutputPort ; "
			"  lv2:index 2 ; lv2:symbol \"latency\" ; lv2:name \"Latency\" ; "
			"  lv2:portProperty lv2:reportsLatency "
			"] .",
			1))
		return 0;

	init_uris();
	SLV2Plugins plugins = slv2_world_get_all_plugins(world);
	SLV2Plugin plug = slv2_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(plug);

	SLV2PluginClass class = slv2_plugin_get_class(plug);
	SLV2Value class_uri = slv2_plugin_class_get_uri(class);
	TEST_ASSERT(!strcmp(slv2_value_as_string(class_uri),
			"http://lv2plug.in/ns/lv2core#CompressorPlugin"));

	SLV2Value plug_bundle_uri = slv2_plugin_get_bundle_uri(plug);
	TEST_ASSERT(!strcmp(slv2_value_as_string(plug_bundle_uri), bundle_dir_uri));
	
	SLV2Values data_uris = slv2_plugin_get_data_uris(plug);
	TEST_ASSERT(slv2_values_size(data_uris) == 2);
	
	char* manifest_uri = (char*)malloc(TEST_PATH_MAX);
	char* data_uri     = (char*)malloc(TEST_PATH_MAX);
	snprintf(manifest_uri, TEST_PATH_MAX, "%s%s",
			slv2_value_as_string(plug_bundle_uri), "manifest.ttl");
	snprintf(data_uri, TEST_PATH_MAX, "%s%s",
			slv2_value_as_string(plug_bundle_uri), "plugin.ttl");
	
	SLV2Value manifest_uri_val = slv2_value_new_uri(world, manifest_uri);
	TEST_ASSERT(slv2_values_contains(data_uris, manifest_uri_val));
	slv2_value_free(manifest_uri_val);

	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_values_get_at(data_uris, 0)), manifest_uri));
	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_values_get_at(data_uris, 1)), data_uri));

	float mins[1];
	float maxs[1];
	float defs[1];
	slv2_plugin_get_port_ranges_float(plug, mins, maxs, defs);
	TEST_ASSERT(mins[0] == -1.0f);
	TEST_ASSERT(maxs[0] == 1.0f);
	TEST_ASSERT(defs[0] == 0.5f);
	
	SLV2Value audio_class = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#AudioPort");
	SLV2Value control_class = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#ControlPort");
	SLV2Value in_class = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#InputPort");
	SLV2Value out_class = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#OutputPort");

	TEST_ASSERT(slv2_plugin_get_num_ports_of_class(plug, control_class, NULL) == 3)
	TEST_ASSERT(slv2_plugin_get_num_ports_of_class(plug, audio_class, NULL) == 0)
	TEST_ASSERT(slv2_plugin_get_num_ports_of_class(plug, in_class, NULL) == 2)
	TEST_ASSERT(slv2_plugin_get_num_ports_of_class(plug, out_class, NULL) == 1)
	TEST_ASSERT(slv2_plugin_get_num_ports_of_class(plug, control_class, in_class, NULL) == 2)
	TEST_ASSERT(slv2_plugin_get_num_ports_of_class(plug, control_class, out_class, NULL) == 1)
	TEST_ASSERT(slv2_plugin_get_num_ports_of_class(plug, audio_class, in_class, NULL) == 0)
	TEST_ASSERT(slv2_plugin_get_num_ports_of_class(plug, audio_class, out_class, NULL) == 0)

	TEST_ASSERT(slv2_plugin_has_latency(plug));
	TEST_ASSERT(slv2_plugin_get_latency_port_index(plug) == 2);

	SLV2Value rt_feature = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#hardRtCapable");
	SLV2Value event_feature = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/ext/event");
	SLV2Value pretend_feature = slv2_value_new_uri(world,
			"http://example.org/solvesWorldHunger");

	TEST_ASSERT(slv2_plugin_has_feature(plug, rt_feature));
	TEST_ASSERT(slv2_plugin_has_feature(plug, event_feature));
	TEST_ASSERT(!slv2_plugin_has_feature(plug, pretend_feature));

	SLV2Values supported = slv2_plugin_get_supported_features(plug);
	SLV2Values required = slv2_plugin_get_required_features(plug);
	SLV2Values optional = slv2_plugin_get_optional_features(plug);
	TEST_ASSERT(slv2_values_size(supported) == 2);
	TEST_ASSERT(slv2_values_size(required) == 1);
	TEST_ASSERT(slv2_values_size(optional) == 1);

	SLV2Value author_name = slv2_plugin_get_author_name(plug);
	TEST_ASSERT(!strcmp(slv2_value_as_string(author_name), "David Robillard"));
	slv2_value_free(author_name);
	
	SLV2Value author_email = slv2_plugin_get_author_email(plug);
	TEST_ASSERT(!strcmp(slv2_value_as_string(author_email), "mailto:dave@drobilla.net"));
	slv2_value_free(author_email);
	
	SLV2Value author_homepage = slv2_plugin_get_author_homepage(plug);
	TEST_ASSERT(!strcmp(slv2_value_as_string(author_homepage), "http://drobilla.net"));
	slv2_value_free(author_homepage);

	slv2_value_free(control_class);
	slv2_value_free(audio_class);
	slv2_value_free(in_class);
	slv2_value_free(out_class);
	cleanup_uris();
	return 1;
}

/*****************************************************************************/

int
test_port()
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES PREFIX_LV2EV
			":plug a lv2:Plugin ; "
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
			"doap:homepage <http://example.org/someplug> ; "
			"lv2:port [ "
			"  a lv2:ControlPort ; a lv2:InputPort ; "
			"  lv2:index 0 ; lv2:symbol \"foo\" ; "
			"  lv2:name \"bar\" ; lv2:name \"le bar\"@fr ; "
     		"  lv2:portProperty lv2:integer ; "
			"  lv2:minimum -1.0 ; lv2:maximum 1.0 ; lv2:default 0.5 ; "
			"  lv2:scalePoint [ rdfs:label \"Sin\"; rdf:value 3 ] ; "
			"  lv2:scalePoint [ rdfs:label \"Cos\"; rdf:value 4 ] "
			"] , [\n"
			"  a lv2:EventPort ; a lv2:InputPort ; "
			"  lv2:index 1 ; lv2:symbol \"event_in\" ; "
			"  lv2:name \"Event Input\" ; "
     		"  lv2ev:supportsEvent <http://example.org/event> "
			"] .",
			1))
		return 0;

	init_uris();
	SLV2Plugins plugins = slv2_world_get_all_plugins(world);
	SLV2Plugin plug = slv2_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(plug);

	SLV2Value psym = slv2_value_new_string(world, "foo");
	SLV2Port p = slv2_plugin_get_port_by_index(plug, 0);
	SLV2Port p2 = slv2_plugin_get_port_by_symbol(plug, psym);
	slv2_value_free(psym);
	TEST_ASSERT(p != NULL);
	TEST_ASSERT(p2 != NULL);
	TEST_ASSERT(p == p2);
	
	SLV2Value nopsym = slv2_value_new_string(world, "thisaintnoportfoo");
	SLV2Port p3 = slv2_plugin_get_port_by_symbol(plug, nopsym);
	TEST_ASSERT(p3 == NULL);
	slv2_value_free(nopsym);

	SLV2Value audio_class = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#AudioPort");
	SLV2Value control_class = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#ControlPort");
	SLV2Value in_class = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#InputPort");

	TEST_ASSERT(slv2_values_size(slv2_port_get_classes(plug, p)) == 2);
	TEST_ASSERT(slv2_plugin_get_num_ports(plug) == 2);
	TEST_ASSERT(slv2_values_get_at(slv2_port_get_classes(plug, p), (unsigned)INT_MAX+1) == NULL);
	TEST_ASSERT(slv2_port_is_a(plug, p, control_class));
	TEST_ASSERT(slv2_port_is_a(plug, p, in_class));
	TEST_ASSERT(!slv2_port_is_a(plug, p, audio_class));
	
	TEST_ASSERT(slv2_values_size(slv2_port_get_properties(plug, p)) == 0);

	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_port_get_symbol(plug, p)), "foo"));
	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_port_get_name(plug, p)), "bar"));

	SLV2ScalePoints points = slv2_port_get_scale_points(plug, p);
	TEST_ASSERT(slv2_scale_points_size(points) == 2);

	TEST_ASSERT(slv2_scale_points_get_at(points, (unsigned)INT_MAX+1) == NULL);
	TEST_ASSERT(slv2_scale_points_get_at(points, 2) == NULL);
	SLV2ScalePoint sp0 = slv2_scale_points_get_at(points, 0);
	TEST_ASSERT(sp0);
	SLV2ScalePoint sp1 = slv2_scale_points_get_at(points, 1);
	TEST_ASSERT(sp1);
	
	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_scale_point_get_label(sp0)), "Sin"));
	TEST_ASSERT(slv2_value_as_float(slv2_scale_point_get_value(sp0)) == 3);
	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_scale_point_get_label(sp1)), "Cos"));
	TEST_ASSERT(slv2_value_as_float(slv2_scale_point_get_value(sp1)) == 4);

	SLV2Value homepage_p = slv2_value_new_uri(world, "http://usefulinc.com/ns/doap#homepage");
	SLV2Values homepages = slv2_plugin_get_value(plug, homepage_p);
	TEST_ASSERT(slv2_values_size(homepages) == 1);
	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_values_get_at(homepages, 0)),
			"http://example.org/someplug"));

	TEST_ASSERT(slv2_plugin_query_count(plug, "SELECT DISTINCT ?bin WHERE {\n"
			   "<> lv2:binary ?bin . }") == 1);
	
	TEST_ASSERT(slv2_plugin_query_count(plug, "SELECT DISTINCT ?parent WHERE {\n"
			   "<> rdfs:subClassOf ?parent . }") == 0);
	
	SLV2Value min, max, def;
	slv2_port_get_range(plug, p, &def, &min, &max);
	TEST_ASSERT(def);
	TEST_ASSERT(min);
	TEST_ASSERT(max);
	TEST_ASSERT(slv2_value_as_float(def) == 0.5);
	TEST_ASSERT(slv2_value_as_float(min) == -1.0);
	TEST_ASSERT(slv2_value_as_float(max) == 1.0);
	
	SLV2Value integer_prop = slv2_value_new_uri(world, "http://lv2plug.in/ns/lv2core#integer");
	SLV2Value toggled_prop = slv2_value_new_uri(world, "http://lv2plug.in/ns/lv2core#toggled");

	TEST_ASSERT(slv2_port_has_property(plug, p, integer_prop));
	TEST_ASSERT(!slv2_port_has_property(plug, p, toggled_prop));
	
	SLV2Port ep = slv2_plugin_get_port_by_index(plug, 1);

	SLV2Value event_type = slv2_value_new_uri(world, "http://example.org/event");
	SLV2Value event_type_2 = slv2_value_new_uri(world, "http://example.org/otherEvent");
	TEST_ASSERT(slv2_port_supports_event(plug, ep, event_type));
	TEST_ASSERT(!slv2_port_supports_event(plug, ep, event_type_2));
	
	SLV2Value name_p = slv2_value_new_uri(world, "http://lv2plug.in/ns/lv2core#name");
	SLV2Values names = slv2_port_get_value(plug, p, name_p);
	TEST_ASSERT(slv2_values_size(names) == 2);
	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_values_get_at(names, 0)),
			"bar"));
	slv2_values_free(names);
	names = slv2_port_get_value(plug, ep, name_p);
	TEST_ASSERT(slv2_values_size(names) == 1);
	TEST_ASSERT(!strcmp(slv2_value_as_string(slv2_values_get_at(names, 0)),
			"Event Input"));
	slv2_values_free(names);

	TEST_ASSERT(slv2_port_get_value(plug, p, min) == NULL);

	slv2_value_free(integer_prop);
	slv2_value_free(toggled_prop);
	slv2_value_free(event_type);
	slv2_value_free(event_type_2);

	slv2_value_free(min);
	slv2_value_free(max);
	slv2_value_free(def);

	slv2_value_free(homepage_p);
	slv2_values_free(homepages);

	slv2_scale_points_free(points);
	slv2_value_free(control_class);
	slv2_value_free(audio_class);
	slv2_value_free(in_class);
	slv2_plugins_free(world, plugins);
	cleanup_uris();
	return 1;
}

/*****************************************************************************/

int
test_ui()
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES PREFIX_LV2UI
			":plug a lv2:Plugin ; a lv2:CompressorPlugin ; "
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
    		"lv2:optionalFeature lv2:hardRtCapable ; "
		    "lv2:requiredFeature <http://lv2plug.in/ns/ext/event> ; "
			"lv2ui:ui :ui , :ui2 , :ui3 , :ui4 ; "
			"doap:maintainer [ foaf:name \"David Robillard\" ; "
			"  foaf:homepage <http://drobilla.net> ; foaf:mbox <mailto:dave@drobilla.net> ] ; "
			"lv2:port [ "
			"  a lv2:ControlPort ; a lv2:InputPort ; "
			"  lv2:index 0 ; lv2:symbol \"foo\" ; lv2:name \"bar\" ; "
			"  lv2:minimum -1.0 ; lv2:maximum 1.0 ; lv2:default 0.5 "
			"] , [ "
			"  a lv2:ControlPort ; a lv2:InputPort ; "
			"  lv2:index 1 ; lv2:symbol \"bar\" ; lv2:name \"Baz\" ; "
			"  lv2:minimum -2.0 ; lv2:maximum 2.0 ; lv2:default 1.0 "
			"] , [ "
			"  a lv2:ControlPort ; a lv2:OutputPort ; "
			"  lv2:index 2 ; lv2:symbol \"latency\" ; lv2:name \"Latency\" ; "
			"  lv2:portProperty lv2:reportsLatency "
			"] .\n"
			":ui a lv2ui:GtkUI ; "
			"  lv2ui:requiredFeature lv2ui:makeResident ; "
			"  lv2ui:binary <ui.so> ; "
			"  lv2ui:optionalFeature lv2ui:ext_presets . "
			":ui2 a lv2ui:GtkUI ; lv2ui:binary <ui2.so> . "
			":ui3 a lv2ui:GtkUI ; lv2ui:binary <ui3.so> . "
			":ui4 a lv2ui:GtkUI ; lv2ui:binary <ui4.so> . ",
			1))
		return 0;

	init_uris();
	SLV2Plugins plugins = slv2_world_get_all_plugins(world);
	SLV2Plugin plug = slv2_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(plug);

	SLV2UIs uis = slv2_plugin_get_uis(plug);
	TEST_ASSERT(slv2_uis_size(uis) == 4);

	TEST_ASSERT(slv2_uis_get_at(uis, (unsigned)INT_MAX + 1) == NULL);

	SLV2UI ui0 = slv2_uis_get_at(uis, 0);
	TEST_ASSERT(ui0);

	SLV2Value ui_uri = slv2_value_new_uri(world, "http://example.org/ui");
	SLV2Value ui2_uri = slv2_value_new_uri(world, "http://example.org/ui3");
	SLV2Value ui3_uri = slv2_value_new_uri(world, "http://example.org/ui4");
	SLV2Value noui_uri = slv2_value_new_uri(world, "http://example.org/notaui");

	SLV2UI ui0_2 = slv2_uis_get_by_uri(uis, ui_uri);
	TEST_ASSERT(ui0 == ui0_2);
	
	SLV2UI ui2 = slv2_uis_get_by_uri(uis, ui2_uri);
	TEST_ASSERT(ui2 != ui0);
	
	SLV2UI ui3 = slv2_uis_get_by_uri(uis, ui3_uri);
	TEST_ASSERT(ui3 != ui0);
	
	SLV2UI noui = slv2_uis_get_by_uri(uis, noui_uri);
	TEST_ASSERT(noui == NULL);
	
	SLV2Values classes = slv2_ui_get_classes(ui0);
	TEST_ASSERT(slv2_values_size(classes) == 1);

	SLV2Value ui_class_uri = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/extensions/ui#GtkUI");

	TEST_ASSERT(slv2_value_equals(slv2_values_get_at(classes, 0), ui_class_uri));
	TEST_ASSERT(slv2_ui_is_a(ui0, ui_class_uri));
	
	SLV2Value plug_bundle_uri = slv2_plugin_get_bundle_uri(plug);
	SLV2Value ui_bundle_uri = slv2_ui_get_bundle_uri(ui0);
	TEST_ASSERT(slv2_value_equals(plug_bundle_uri, ui_bundle_uri));

	char* ui_binary_uri_str = (char*)malloc(TEST_PATH_MAX);
	snprintf(ui_binary_uri_str, TEST_PATH_MAX, "%s%s",
			slv2_value_as_string(plug_bundle_uri), "ui.so");

	SLV2Value ui_binary_uri = slv2_ui_get_binary_uri(ui0);

	SLV2Value expected_uri = slv2_value_new_uri(world, ui_binary_uri_str);
	TEST_ASSERT(slv2_value_equals(expected_uri, ui_binary_uri));

	slv2_value_free(ui_uri);
	slv2_value_free(ui2_uri);
	slv2_value_free(noui_uri);
	slv2_value_free(expected_uri);
	slv2_uis_free(uis);

	cleanup_uris();
	return 1;
}

/*****************************************************************************/

/* add tests here */
static struct TestCase tests[] = {
	TEST_CASE(utils),
	TEST_CASE(value),
	TEST_CASE(values),
	/* TEST_CASE(discovery_load_bundle), */
	TEST_CASE(verify),
	TEST_CASE(no_verify),
	TEST_CASE(discovery_load_all),
	TEST_CASE(classes),
	TEST_CASE(plugin),
	TEST_CASE(port),
	TEST_CASE(plugin),
	TEST_CASE(ui),
	{ NULL, NULL }
};

void
run_tests()
{
	int i;
	for (i = 0; tests[i].title; i++) {
		printf("--- Test: %s\n", tests[i].title);
		if (!tests[i].func()) {
			printf("\nTest failed\n");
			/* test case that wasn't able to be executed at all counts as 1 test + 1 error */
			error_count++;
			test_count++;
		}
		unload_bundle();
		cleanup();
	}
}

int
main(int argc, char *argv[])
{
	if (argc != 1) {
		printf("Syntax: %s\n", argv[0]);
		return 0;
	}
	init_tests();
	run_tests();
	cleanup();
	printf("\n***\n*** Test Results: %d tests, %d errors\n***\n\n", test_count, error_count);
	return error_count ? 1 : 0;
}

