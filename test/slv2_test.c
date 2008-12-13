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

#define PREFIX_LINE "@prefix : <http://example.com/> .\n"
#define PREFIX_LV2 "@prefix lv2: <http://lv2plug.in/ns/lv2core#> .\n"
#define PREFIX_RDFS "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
#define PREFIX_FOAF "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
#define PREFIX_DOAP "@prefix doap: <http://usefulinc.com/ns/doap#> .\n"

#define MANIFEST_PREFIXES PREFIX_LINE PREFIX_LV2 PREFIX_RDFS
#define BUNDLE_PREFIXES PREFIX_LINE PREFIX_LV2 PREFIX_RDFS PREFIX_FOAF PREFIX_DOAP
#define PLUGIN_NAME(name) "doap:name \"" name "\""
#define LICENSE_GPL "doap:license <http://usefulinc.com/doap/licenses/gpl>"

static char *uris_plugin = "http://example.com/plug";
static SLV2Value plugin_uri_value, plugin2_uri_value;

/*****************************************************************************/

void
init_uris()
{
	plugin_uri_value = slv2_value_new_uri(world, uris_plugin);
	plugin2_uri_value = slv2_value_new_uri(world, "http://example.com/foobar");
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
	TEST_ASSERT(!slv2_uri_to_path("file:/example.com/blah"));
	TEST_ASSERT(!slv2_uri_to_path("http://example.com/blah"));
	return 1;
}

/*****************************************************************************/

int
test_value()
{
	const char *uri = "http://example.com/";
	char *res;

	init_world();
	SLV2Value v1 = slv2_value_new_uri(world, "http://example.com/");
	TEST_ASSERT(v1);
	TEST_ASSERT(slv2_value_is_uri(v1));
	TEST_ASSERT(!strcmp(slv2_value_as_uri(v1), uri));
	TEST_ASSERT(!slv2_value_is_literal(v1));
	TEST_ASSERT(!slv2_value_is_string(v1));
	TEST_ASSERT(!slv2_value_is_float(v1));
	TEST_ASSERT(!slv2_value_is_int(v1));
	res = slv2_value_get_turtle_token(v1);
	TEST_ASSERT(!strcmp(res, "<http://example.com/>"));

	SLV2Value v2 = slv2_value_new_uri(world, uri);
	TEST_ASSERT(v2);
	TEST_ASSERT(slv2_value_is_uri(v2));
	TEST_ASSERT(!strcmp(slv2_value_as_uri(v2), uri));

	TEST_ASSERT(slv2_value_equals(v1, v2));

	SLV2Value v3 = slv2_value_new_uri(world, "http://example.com/another");
	TEST_ASSERT(v3);
	TEST_ASSERT(slv2_value_is_uri(v3));
	TEST_ASSERT(!strcmp(slv2_value_as_uri(v3), "http://example.com/another"));
	TEST_ASSERT(!slv2_value_equals(v1, v3));

	slv2_value_free(v2);
	v2 = slv2_value_duplicate(v1);
	TEST_ASSERT(slv2_value_equals(v1, v2));
	TEST_ASSERT(slv2_value_is_uri(v2));
	TEST_ASSERT(!strcmp(slv2_value_as_uri(v2), uri));
	TEST_ASSERT(!slv2_value_is_literal(v2));
	TEST_ASSERT(!slv2_value_is_string(v2));
	TEST_ASSERT(!slv2_value_is_float(v2));
	TEST_ASSERT(!slv2_value_is_int(v2));

	slv2_value_free(v3);
	slv2_value_free(v2);
	slv2_value_free(v1);
	return 1;
}

/*****************************************************************************/

int
test_values()
{
	init_world();
	SLV2Value v0 = slv2_value_new_uri(world, "http://example.com/");
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
	slv2_plugin_verify(explug);
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

	slv2_value_free(control_class);
	slv2_value_free(audio_class);
	slv2_value_free(in_class);
	slv2_value_free(out_class);
	slv2_plugins_free(world, plugins);
	cleanup_uris();
	return 1;
}

/*****************************************************************************/

int
test_port()
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin ; "
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
			"doap:homepage <http://example.org/someplug> ; "
			"lv2:port [ "
			"  a lv2:ControlPort ; a lv2:InputPort ; "
			"  lv2:index 0 ; lv2:symbol \"foo\" ; "
			"  lv2:name \"bar\" ; lv2:name \"le bar\"@fr ; "
			"  lv2:scalePoint [ rdfs:label \"Sin\"; rdf:value 3 ] ; "
			"  lv2:scalePoint [ rdfs:label \"Cos\"; rdf:value 4 ] ; "
			"] .",
			1))
		return 0;

	init_uris();
	SLV2Plugins plugins = slv2_world_get_all_plugins(world);
	SLV2Plugin plug = slv2_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(plug);

	SLV2Port p = slv2_plugin_get_port_by_index(plug, 0);
	//SLV2Port p2 = slv2_plugin_get_port_by_symbol(plug, "foo");
	TEST_ASSERT(p != NULL);
	//TEST_ASSERT(p2 != NULL);
	//TEST_ASSERT(p == p2);

	SLV2Value audio_class = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#AudioPort");
	SLV2Value control_class = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#ControlPort");
	SLV2Value in_class = slv2_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#InputPort");

	TEST_ASSERT(slv2_values_size(slv2_port_get_classes(plug, p)) == 2);
	TEST_ASSERT(slv2_plugin_get_num_ports(plug) == 1);
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

/* add tests here */
static struct TestCase tests[] = {
	TEST_CASE(utils),
	TEST_CASE(value),
	TEST_CASE(values),
	/* TEST_CASE(discovery_load_bundle), */
	TEST_CASE(verify),
	TEST_CASE(discovery_load_all),
	TEST_CASE(classes),
	TEST_CASE(plugin),
	TEST_CASE(port),
	TEST_CASE(plugin),
	{ NULL, NULL }
};

void
run_tests()
{
	int i;
	for (i = 0; tests[i].title; i++) {
		printf("\n--- Test: %s\n", tests[i].title);
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
	printf("\n--- Results: %d tests, %d errors\n", test_count, error_count);
	return error_count ? 1 : 0;
}

