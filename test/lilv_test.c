/* Lilv Tests
 * Copyright 2008-2011 David Robillard <http://drobilla.net>
 * Copyright 2008 Krzysztof Foltman
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

#define _XOPEN_SOURCE 600

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#include "lilv/lilv.h"

#define TEST_PATH_MAX 1024

static char bundle_dir_name[TEST_PATH_MAX];
static char bundle_dir_uri[TEST_PATH_MAX];
static char manifest_name[TEST_PATH_MAX];
static char content_name[TEST_PATH_MAX];

static LilvWorld world;

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
	strcat(bundle_dir_name, "/.lv2/lilv-test.lv2");
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
	world = lilv_world_new();
	return world != NULL;
}

int
load_all_bundles()
{
	if (!init_world())
		return 0;
	lilv_world_load_all(world);
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
start_bundle(char *manifest, char *content)
{
	create_bundle(manifest, content);
	return load_all_bundles();
}

void
unload_bundle()
{
	if (world)
		lilv_world_free(world);
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
		fprintf(stderr, "Failure at lilv_test.c:%d: %s\n", __LINE__, #check);\
	}\
} while (0)

typedef int (*TestFunc)();

struct TestCase {
	const char *title;
	TestFunc func;
};

#define PREFIX_LINE "@prefix : <http://example.org/> .\n"
#define PREFIX_LV2 "@prefix lv2: <http://lv2plug.in/ns/lv2core#> .\n"
#define PREFIX_LV2EV "@prefix lv2ev: <http://lv2plug.in/ns/ext/event#> . \n"
#define PREFIX_LV2UI "@prefix lv2ui: <http://lv2plug.in/ns/extensions/ui#> .\n"
#define PREFIX_RDF "@prefix rdf:  <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
#define PREFIX_RDFS "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
#define PREFIX_FOAF "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
#define PREFIX_DOAP "@prefix doap: <http://usefulinc.com/ns/doap#> .\n"

#define MANIFEST_PREFIXES PREFIX_LINE PREFIX_LV2 PREFIX_RDFS
#define BUNDLE_PREFIXES PREFIX_LINE PREFIX_LV2 PREFIX_RDF PREFIX_RDFS PREFIX_FOAF PREFIX_DOAP
#define PLUGIN_NAME(name) "doap:name \"" name "\""
#define LICENSE_GPL "doap:license <http://usefulinc.com/doap/licenses/gpl>"

static char *uris_plugin = "http://example.org/plug";
static LilvValue plugin_uri_value, plugin2_uri_value;

/*****************************************************************************/

void
init_uris()
{
	plugin_uri_value = lilv_value_new_uri(world, uris_plugin);
	plugin2_uri_value = lilv_value_new_uri(world, "http://example.org/foobar");
	TEST_ASSERT(plugin_uri_value);
	TEST_ASSERT(plugin2_uri_value);
}

void
cleanup_uris()
{
	lilv_value_free(plugin2_uri_value);
	lilv_value_free(plugin_uri_value);
	plugin2_uri_value = NULL;
	plugin_uri_value = NULL;
}

/*****************************************************************************/

int
test_utils()
{
	TEST_ASSERT(!strcmp(lilv_uri_to_path("file:///tmp/blah"), "/tmp/blah"));
	TEST_ASSERT(!lilv_uri_to_path("file:/example.org/blah"));
	TEST_ASSERT(!lilv_uri_to_path("http://example.org/blah"));
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
			"] ."))
		return 0;

	init_uris();

	LilvValue uval = lilv_value_new_uri(world, "http://example.org");
	LilvValue sval = lilv_value_new_string(world, "Foo");
	LilvValue ival = lilv_value_new_int(world, 42);
	LilvValue fval = lilv_value_new_float(world, 1.6180);

	TEST_ASSERT(lilv_value_is_uri(uval));
	TEST_ASSERT(lilv_value_is_string(sval));
	TEST_ASSERT(lilv_value_is_int(ival));
	TEST_ASSERT(lilv_value_is_float(fval));

	TEST_ASSERT(!lilv_value_is_literal(uval));
	TEST_ASSERT(lilv_value_is_literal(sval));
	TEST_ASSERT(lilv_value_is_literal(ival));
	TEST_ASSERT(lilv_value_is_literal(fval));

	TEST_ASSERT(!strcmp(lilv_value_as_uri(uval), "http://example.org"));
	TEST_ASSERT(!strcmp(lilv_value_as_string(sval), "Foo"));
	TEST_ASSERT(lilv_value_as_int(ival) == 42);
	TEST_ASSERT(fabs(lilv_value_as_float(fval) - 1.6180) < FLT_EPSILON);

	char* tok = lilv_value_get_turtle_token(uval);
	TEST_ASSERT(!strcmp(tok, "<http://example.org>"));
	free(tok);
	tok = lilv_value_get_turtle_token(sval);
	TEST_ASSERT(!strcmp(tok, "Foo"));
	free(tok);
	tok = lilv_value_get_turtle_token(ival);
	TEST_ASSERT(!strcmp(tok, "42"));
	free(tok);
	tok = lilv_value_get_turtle_token(fval);
	TEST_ASSERT(!strncmp(tok, "1.6180", 6));
	free(tok);

	LilvValue uval_e = lilv_value_new_uri(world, "http://example.org");
	LilvValue sval_e = lilv_value_new_string(world, "Foo");
	LilvValue ival_e = lilv_value_new_int(world, 42);
	LilvValue fval_e = lilv_value_new_float(world, 1.6180);
	LilvValue uval_ne = lilv_value_new_uri(world, "http://no-example.org");
	LilvValue sval_ne = lilv_value_new_string(world, "Bar");
	LilvValue ival_ne = lilv_value_new_int(world, 24);
	LilvValue fval_ne = lilv_value_new_float(world, 3.14159);

	TEST_ASSERT(lilv_value_equals(uval, uval_e));
	TEST_ASSERT(lilv_value_equals(sval, sval_e));
	TEST_ASSERT(lilv_value_equals(ival, ival_e));
	TEST_ASSERT(lilv_value_equals(fval, fval_e));

	TEST_ASSERT(!lilv_value_equals(uval, uval_ne));
	TEST_ASSERT(!lilv_value_equals(sval, sval_ne));
	TEST_ASSERT(!lilv_value_equals(ival, ival_ne));
	TEST_ASSERT(!lilv_value_equals(fval, fval_ne));

	TEST_ASSERT(!lilv_value_equals(uval, sval));
	TEST_ASSERT(!lilv_value_equals(sval, ival));
	TEST_ASSERT(!lilv_value_equals(ival, fval));

	LilvValue uval_dup = lilv_value_duplicate(uval);
	TEST_ASSERT(lilv_value_equals(uval, uval_dup));

	LilvValue ifval = lilv_value_new_float(world, 42.0);
	TEST_ASSERT(!lilv_value_equals(ival, ifval));
	lilv_value_free(ifval);

	LilvValue nil = NULL;
	TEST_ASSERT(!lilv_value_equals(uval, nil));
	TEST_ASSERT(!lilv_value_equals(nil, uval));
	TEST_ASSERT(lilv_value_equals(nil, nil));

	LilvValue nil2 = lilv_value_duplicate(nil);
	TEST_ASSERT(lilv_value_equals(nil, nil2));

	lilv_value_free(uval);
	lilv_value_free(sval);
	lilv_value_free(ival);
	lilv_value_free(fval);
	lilv_value_free(uval_e);
	lilv_value_free(sval_e);
	lilv_value_free(ival_e);
	lilv_value_free(fval_e);
	lilv_value_free(uval_ne);
	lilv_value_free(sval_ne);
	lilv_value_free(ival_ne);
	lilv_value_free(fval_ne);
	lilv_value_free(uval_dup);
	lilv_value_free(nil2);

	cleanup_uris();
	return 1;
}

/*****************************************************************************/

static int discovery_plugin_found = 0;

static void
discovery_verify_plugin(LilvPlugin plugin)
{
	LilvValue value = lilv_plugin_get_uri(plugin);
	if (lilv_value_equals(value, plugin_uri_value)) {
		LilvValue lib_uri = NULL;
		TEST_ASSERT(!lilv_value_equals(value, plugin2_uri_value));
		discovery_plugin_found = 1;
		lib_uri = lilv_plugin_get_library_uri(plugin);
		TEST_ASSERT(lib_uri);
		TEST_ASSERT(lilv_value_is_uri(lib_uri));
		TEST_ASSERT(lilv_value_as_uri(lib_uri));
		TEST_ASSERT(strstr(lilv_value_as_uri(lib_uri), "foo.so"));
		TEST_ASSERT(lilv_plugin_verify(plugin));
	}
}

int
test_discovery()
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin ;"
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
			"lv2:port [ a lv2:ControlPort ; a lv2:InputPort ;"
			" lv2:index 0 ; lv2:symbol \"foo\" ; lv2:name \"bar\" ; ] ."))
		return 0;

	init_uris();

	LilvPlugins plugins = lilv_world_get_all_plugins(world);
	TEST_ASSERT(lilv_plugins_size(plugins) > 0);

	LilvPlugin explug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(explug != NULL);
	LilvPlugin explug2 = lilv_plugins_get_by_uri(plugins, plugin2_uri_value);
	TEST_ASSERT(explug2 == NULL);

	if (explug) {
		LilvValue name = lilv_plugin_get_name(explug);
		TEST_ASSERT(!strcmp(lilv_value_as_string(name), "Test plugin"));
		lilv_value_free(name);
	}

	discovery_plugin_found = 0;
	LILV_FOREACH(plugins, i, plugins)
		discovery_verify_plugin(lilv_plugins_get(plugins, i));

	TEST_ASSERT(discovery_plugin_found);
	plugins = NULL;

	cleanup_uris();

	return 1;
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
			" lv2:index 0 ; lv2:symbol \"foo\" ; lv2:name \"bar\" ] ."))
		return 0;

	init_uris();
	LilvPlugins plugins = lilv_world_get_all_plugins(world);
	LilvPlugin explug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(explug);
	TEST_ASSERT(lilv_plugin_verify(explug));
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
			":plug a lv2:Plugin . "))
		return 0;

	init_uris();
	LilvPlugins plugins = lilv_world_get_all_plugins(world);
	LilvPlugin explug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(explug);
	TEST_ASSERT(!lilv_plugin_verify(explug));
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
			"] ."))
		return 0;

	init_uris();
	LilvPluginClass plugin = lilv_world_get_plugin_class(world);
	LilvPluginClasses classes = lilv_world_get_plugin_classes(world);
	LilvPluginClasses children = lilv_plugin_class_get_children(plugin);

	TEST_ASSERT(lilv_plugin_class_get_parent_uri(plugin) == NULL);
	TEST_ASSERT(lilv_plugin_classes_size(classes) > lilv_plugin_classes_size(children));
	TEST_ASSERT(!strcmp(lilv_value_as_string(lilv_plugin_class_get_label(plugin)), "Plugin"));
	TEST_ASSERT(!strcmp(lilv_value_as_string(lilv_plugin_class_get_uri(plugin)),
			"http://lv2plug.in/ns/lv2core#Plugin"));

	LILV_FOREACH(plugin_classes, i, children) {
		TEST_ASSERT(lilv_value_equals(
				lilv_plugin_class_get_parent_uri(lilv_plugin_classes_get(children, i)),
				lilv_plugin_class_get_uri(plugin)));
	}

	LilvValue some_uri = lilv_value_new_uri(world, "http://example.org/whatever");
	TEST_ASSERT(lilv_plugin_classes_get_by_uri(classes, some_uri) == NULL);
	lilv_value_free(some_uri);

	lilv_plugin_classes_free(children);

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
			"lv2:optionalFeature lv2:hardRTCapable ; "
		    "lv2:requiredFeature <http://lv2plug.in/ns/ext/event> ; "
			":foo 1.6180 ; "
			":bar true ; "
			":baz false ; "
			"doap:maintainer [ foaf:name \"David Robillard\" ; "
			"  foaf:homepage <http://drobilla.net> ; foaf:mbox <mailto:d@drobilla.net> ] ; "
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
			"] . \n"
			":thing doap:name \"Something else\" .\n"))
		return 0;

	init_uris();
	LilvPlugins plugins = lilv_world_get_all_plugins(world);
	LilvPlugin plug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(plug);

	LilvPluginClass class = lilv_plugin_get_class(plug);
	LilvValue class_uri = lilv_plugin_class_get_uri(class);
	TEST_ASSERT(!strcmp(lilv_value_as_string(class_uri),
			"http://lv2plug.in/ns/lv2core#CompressorPlugin"));

	LilvValue plug_bundle_uri = lilv_plugin_get_bundle_uri(plug);
	TEST_ASSERT(!strcmp(lilv_value_as_string(plug_bundle_uri), bundle_dir_uri));

	LilvValues data_uris = lilv_plugin_get_data_uris(plug);
	TEST_ASSERT(lilv_values_size(data_uris) == 2);

	char* manifest_uri = (char*)malloc(TEST_PATH_MAX);
	char* data_uri     = (char*)malloc(TEST_PATH_MAX);
	snprintf(manifest_uri, TEST_PATH_MAX, "%s%s",
			lilv_value_as_string(plug_bundle_uri), "manifest.ttl");
	snprintf(data_uri, TEST_PATH_MAX, "%s%s",
			lilv_value_as_string(plug_bundle_uri), "plugin.ttl");

	LilvValue manifest_uri_val = lilv_value_new_uri(world, manifest_uri);
	TEST_ASSERT(lilv_values_contains(data_uris, manifest_uri_val));
	lilv_value_free(manifest_uri_val);

	LilvValue data_uri_val = lilv_value_new_uri(world, data_uri);
	TEST_ASSERT(lilv_values_contains(data_uris, data_uri_val));
	lilv_value_free(data_uri_val);

	free(manifest_uri);
	free(data_uri);

	float mins[1];
	float maxs[1];
	float defs[1];
	lilv_plugin_get_port_ranges_float(plug, mins, maxs, defs);
	TEST_ASSERT(mins[0] == -1.0f);
	TEST_ASSERT(maxs[0] == 1.0f);
	TEST_ASSERT(defs[0] == 0.5f);

	LilvValue audio_class = lilv_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#AudioPort");
	LilvValue control_class = lilv_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#ControlPort");
	LilvValue in_class = lilv_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#InputPort");
	LilvValue out_class = lilv_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#OutputPort");

	TEST_ASSERT(lilv_plugin_get_num_ports_of_class(plug, control_class, NULL) == 3);
	TEST_ASSERT(lilv_plugin_get_num_ports_of_class(plug, audio_class, NULL) == 0);
	TEST_ASSERT(lilv_plugin_get_num_ports_of_class(plug, in_class, NULL) == 2);
	TEST_ASSERT(lilv_plugin_get_num_ports_of_class(plug, out_class, NULL) == 1);
	TEST_ASSERT(lilv_plugin_get_num_ports_of_class(plug, control_class, in_class, NULL) == 2);
	TEST_ASSERT(lilv_plugin_get_num_ports_of_class(plug, control_class, out_class, NULL) == 1);
	TEST_ASSERT(lilv_plugin_get_num_ports_of_class(plug, audio_class, in_class, NULL) == 0);
	TEST_ASSERT(lilv_plugin_get_num_ports_of_class(plug, audio_class, out_class, NULL) == 0);

	TEST_ASSERT(lilv_plugin_has_latency(plug));
	TEST_ASSERT(lilv_plugin_get_latency_port_index(plug) == 2);

	LilvValue rt_feature = lilv_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#hardRTCapable");
	LilvValue event_feature = lilv_value_new_uri(world,
			"http://lv2plug.in/ns/ext/event");
	LilvValue pretend_feature = lilv_value_new_uri(world,
			"http://example.org/solvesWorldHunger");

	TEST_ASSERT(lilv_plugin_has_feature(plug, rt_feature));
	TEST_ASSERT(lilv_plugin_has_feature(plug, event_feature));
	TEST_ASSERT(!lilv_plugin_has_feature(plug, pretend_feature));

	lilv_value_free(rt_feature);
	lilv_value_free(event_feature);
	lilv_value_free(pretend_feature);

	LilvValues supported = lilv_plugin_get_supported_features(plug);
	LilvValues required = lilv_plugin_get_required_features(plug);
	LilvValues optional = lilv_plugin_get_optional_features(plug);
	TEST_ASSERT(lilv_values_size(supported) == 2);
	TEST_ASSERT(lilv_values_size(required) == 1);
	TEST_ASSERT(lilv_values_size(optional) == 1);
	lilv_values_free(supported);
	lilv_values_free(required);
	lilv_values_free(optional);

	LilvValue  foo_p = lilv_value_new_uri(world, "http://example.org/foo");
	LilvValues foos  = lilv_plugin_get_value(plug, foo_p);
	TEST_ASSERT(lilv_values_size(foos) == 1);
	TEST_ASSERT(fabs(lilv_value_as_float(lilv_values_get_first(foos)) - 1.6180) < FLT_EPSILON);
	lilv_value_free(foo_p);
	lilv_values_free(foos);

	LilvValue  bar_p = lilv_value_new_uri(world, "http://example.org/bar");
	LilvValues bars  = lilv_plugin_get_value(plug, bar_p);
	TEST_ASSERT(lilv_values_size(bars) == 1);
	TEST_ASSERT(lilv_value_as_bool(lilv_values_get_first(bars)) == true);
	lilv_value_free(bar_p);
	lilv_values_free(bars);

	LilvValue  baz_p = lilv_value_new_uri(world, "http://example.org/baz");
	LilvValues bazs  = lilv_plugin_get_value(plug, baz_p);
	TEST_ASSERT(lilv_values_size(bazs) == 1);
	TEST_ASSERT(lilv_value_as_bool(lilv_values_get_first(bazs)) == false);
	lilv_value_free(baz_p);
	lilv_values_free(bazs);

	LilvValue author_name = lilv_plugin_get_author_name(plug);
	TEST_ASSERT(!strcmp(lilv_value_as_string(author_name), "David Robillard"));
	lilv_value_free(author_name);

	LilvValue author_email = lilv_plugin_get_author_email(plug);
	TEST_ASSERT(!strcmp(lilv_value_as_string(author_email), "mailto:d@drobilla.net"));
	lilv_value_free(author_email);

	LilvValue author_homepage = lilv_plugin_get_author_homepage(plug);
	TEST_ASSERT(!strcmp(lilv_value_as_string(author_homepage), "http://drobilla.net"));
	lilv_value_free(author_homepage);

	LilvValue thing_uri = lilv_value_new_uri(world, "http://example.org/thing");
	LilvValue name_p = lilv_value_new_uri(world, "http://usefulinc.com/ns/doap#name");
	LilvValues thing_names = lilv_plugin_get_value_for_subject(plug, thing_uri, name_p);
	TEST_ASSERT(lilv_values_size(thing_names) == 1);
	LilvValue thing_name = lilv_values_get_first(thing_names);
	TEST_ASSERT(thing_name);
	TEST_ASSERT(lilv_value_is_string(thing_name));
	TEST_ASSERT(!strcmp(lilv_value_as_string(thing_name), "Something else"));

	LilvUIs uis = lilv_plugin_get_uis(plug);
	TEST_ASSERT(lilv_uis_size(uis) == 0);
	lilv_uis_free(uis);

	lilv_values_free(thing_names);
	lilv_value_free(thing_uri);
	lilv_value_free(name_p);
	lilv_value_free(control_class);
	lilv_value_free(audio_class);
	lilv_value_free(in_class);
	lilv_value_free(out_class);
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
			"  lv2:name \"store\" ; "
			"  lv2:name \"dépanneur\"@fr-ca ; lv2:name \"épicerie\"@fr-fr ; "
			"  lv2:name \"tienda\"@es ; "
     		"  lv2:portProperty lv2:integer ; "
			"  lv2:minimum -1.0 ; lv2:maximum 1.0 ; lv2:default 0.5 ; "
			"  lv2:scalePoint [ rdfs:label \"Sin\"; rdf:value 3 ] ; "
			"  lv2:scalePoint [ rdfs:label \"Cos\"; rdf:value 4 ] "
			"] , [\n"
			"  a lv2:EventPort ; a lv2:InputPort ; "
			"  lv2:index 1 ; lv2:symbol \"event_in\" ; "
			"  lv2:name \"Event Input\" ; "
     		"  lv2ev:supportsEvent <http://example.org/event> "
			"] ."))
		return 0;

	init_uris();
	LilvPlugins plugins = lilv_world_get_all_plugins(world);
	LilvPlugin plug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(plug);

	LilvValue psym = lilv_value_new_string(world, "foo");
	LilvPort p = lilv_plugin_get_port_by_index(plug, 0);
	LilvPort p2 = lilv_plugin_get_port_by_symbol(plug, psym);
	lilv_value_free(psym);
	TEST_ASSERT(p != NULL);
	TEST_ASSERT(p2 != NULL);
	TEST_ASSERT(p == p2);

	LilvValue nopsym = lilv_value_new_string(world, "thisaintnoportfoo");
	LilvPort p3 = lilv_plugin_get_port_by_symbol(plug, nopsym);
	TEST_ASSERT(p3 == NULL);
	lilv_value_free(nopsym);

	LilvValue audio_class = lilv_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#AudioPort");
	LilvValue control_class = lilv_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#ControlPort");
	LilvValue in_class = lilv_value_new_uri(world,
			"http://lv2plug.in/ns/lv2core#InputPort");

	TEST_ASSERT(lilv_values_size(lilv_port_get_classes(plug, p)) == 2);
	TEST_ASSERT(lilv_plugin_get_num_ports(plug) == 2);
	TEST_ASSERT(lilv_port_is_a(plug, p, control_class));
	TEST_ASSERT(lilv_port_is_a(plug, p, in_class));
	TEST_ASSERT(!lilv_port_is_a(plug, p, audio_class));

	LilvValues port_properties = lilv_port_get_properties(plug, p);
	TEST_ASSERT(lilv_values_size(port_properties) == 1);
	lilv_values_free(port_properties);

	// Untranslated name (current locale is set to "C" in main)
	TEST_ASSERT(!strcmp(lilv_value_as_string(lilv_port_get_symbol(plug, p)), "foo"));
	LilvValue name = lilv_port_get_name(plug, p);
	TEST_ASSERT(!strcmp(lilv_value_as_string(name), "store"));
	lilv_value_free(name);

	// Exact language match
	setenv("LANG", "fr_FR", 1);
	name = lilv_port_get_name(plug, p);
	TEST_ASSERT(!strcmp(lilv_value_as_string(name), "épicerie"));
	lilv_value_free(name);

	// Exact language match (with charset suffix)
	setenv("LANG", "fr_CA.utf8", 1);
	name = lilv_port_get_name(plug, p);
	TEST_ASSERT(!strcmp(lilv_value_as_string(name), "dépanneur"));
	lilv_value_free(name);

	// Partial language match (choose value translated for different country)
	setenv("LANG", "fr_BE", 1);
	name = lilv_port_get_name(plug, p);
	TEST_ASSERT((!strcmp(lilv_value_as_string(name), "dépanneur"))
	            ||(!strcmp(lilv_value_as_string(name), "épicerie")));
	lilv_value_free(name);

	// Partial language match (choose country-less language tagged value)
	setenv("LANG", "es_MX", 1);
	name = lilv_port_get_name(plug, p);
	TEST_ASSERT(!strcmp(lilv_value_as_string(name), "tienda"));
	lilv_value_free(name);

	setenv("LANG", "C", 1);  // Reset locale

	LilvScalePoints points = lilv_port_get_scale_points(plug, p);
	TEST_ASSERT(lilv_scale_points_size(points) == 2);

	LilvIter sp_iter = lilv_scale_points_begin(points);
	LilvScalePoint sp0 = lilv_scale_points_get(points, sp_iter);
	TEST_ASSERT(sp0);
	lilv_scale_points_next(points, sp_iter);
	LilvScalePoint sp1 = lilv_scale_points_get(points, sp_iter);
	TEST_ASSERT(sp1);

	TEST_ASSERT(!strcmp(lilv_value_as_string(lilv_scale_point_get_label(sp0)), "Sin"));
	TEST_ASSERT(lilv_value_as_float(lilv_scale_point_get_value(sp0)) == 3);
	TEST_ASSERT(!strcmp(lilv_value_as_string(lilv_scale_point_get_label(sp1)), "Cos"));
	TEST_ASSERT(lilv_value_as_float(lilv_scale_point_get_value(sp1)) == 4);

	LilvValue homepage_p = lilv_value_new_uri(world, "http://usefulinc.com/ns/doap#homepage");
	LilvValues homepages = lilv_plugin_get_value(plug, homepage_p);
	TEST_ASSERT(lilv_values_size(homepages) == 1);
	TEST_ASSERT(!strcmp(lilv_value_as_string(lilv_values_get_first(homepages)),
			"http://example.org/someplug"));

	LilvValue min, max, def;
	lilv_port_get_range(plug, p, &def, &min, &max);
	TEST_ASSERT(def);
	TEST_ASSERT(min);
	TEST_ASSERT(max);
	TEST_ASSERT(lilv_value_as_float(def) == 0.5);
	TEST_ASSERT(lilv_value_as_float(min) == -1.0);
	TEST_ASSERT(lilv_value_as_float(max) == 1.0);

	LilvValue integer_prop = lilv_value_new_uri(world, "http://lv2plug.in/ns/lv2core#integer");
	LilvValue toggled_prop = lilv_value_new_uri(world, "http://lv2plug.in/ns/lv2core#toggled");

	TEST_ASSERT(lilv_port_has_property(plug, p, integer_prop));
	TEST_ASSERT(!lilv_port_has_property(plug, p, toggled_prop));

	LilvPort ep = lilv_plugin_get_port_by_index(plug, 1);

	LilvValue event_type = lilv_value_new_uri(world, "http://example.org/event");
	LilvValue event_type_2 = lilv_value_new_uri(world, "http://example.org/otherEvent");
	TEST_ASSERT(lilv_port_supports_event(plug, ep, event_type));
	TEST_ASSERT(!lilv_port_supports_event(plug, ep, event_type_2));

	LilvValue name_p = lilv_value_new_uri(world, "http://lv2plug.in/ns/lv2core#name");
	LilvValues names = lilv_port_get_value(plug, p, name_p);
	TEST_ASSERT(lilv_values_size(names) == 1);
	TEST_ASSERT(!strcmp(lilv_value_as_string(lilv_values_get_first(names)),
	                    "store"));
	lilv_values_free(names);

	LilvValue true_val  = lilv_value_new_bool(world, true);
	LilvValue false_val = lilv_value_new_bool(world, false);

	lilv_world_set_option(world, LILV_OPTION_FILTER_LANG, false_val);
	names = lilv_port_get_value(plug, p, name_p);
	TEST_ASSERT(lilv_values_size(names) == 4);
	lilv_values_free(names);
	lilv_world_set_option(world, LILV_OPTION_FILTER_LANG, true_val);

	lilv_value_free(false_val);
	lilv_value_free(true_val);

	names = lilv_port_get_value(plug, ep, name_p);
	TEST_ASSERT(lilv_values_size(names) == 1);
	TEST_ASSERT(!strcmp(lilv_value_as_string(lilv_values_get_first(names)),
	                    "Event Input"));
	lilv_values_free(names);
	lilv_value_free(name_p);

	lilv_value_free(integer_prop);
	lilv_value_free(toggled_prop);
	lilv_value_free(event_type);
	lilv_value_free(event_type_2);

	lilv_value_free(min);
	lilv_value_free(max);
	lilv_value_free(def);

	lilv_value_free(homepage_p);
	lilv_values_free(homepages);

	lilv_scale_points_free(points);
	lilv_value_free(control_class);
	lilv_value_free(audio_class);
	lilv_value_free(in_class);
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
			"lv2:optionalFeature lv2:hardRTCapable ; "
		    "lv2:requiredFeature <http://lv2plug.in/ns/ext/event> ; "
			"lv2ui:ui :ui , :ui2 , :ui3 , :ui4 ; "
			"doap:maintainer [ foaf:name \"David Robillard\" ; "
			"  foaf:homepage <http://drobilla.net> ; foaf:mbox <mailto:d@drobilla.net> ] ; "
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
			":ui4 a lv2ui:GtkUI ; lv2ui:binary <ui4.so> . "))
		return 0;

	init_uris();
	LilvPlugins plugins = lilv_world_get_all_plugins(world);
	LilvPlugin plug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(plug);

	LilvUIs uis = lilv_plugin_get_uis(plug);
	TEST_ASSERT(lilv_uis_size(uis) == 4);

	LilvUI ui0 = lilv_uis_get(uis, lilv_uis_begin(uis));
	TEST_ASSERT(ui0);

	LilvValue ui_uri = lilv_value_new_uri(world, "http://example.org/ui");
	LilvValue ui2_uri = lilv_value_new_uri(world, "http://example.org/ui3");
	LilvValue ui3_uri = lilv_value_new_uri(world, "http://example.org/ui4");
	LilvValue noui_uri = lilv_value_new_uri(world, "http://example.org/notaui");

	LilvUI ui0_2 = lilv_uis_get_by_uri(uis, ui_uri);
	TEST_ASSERT(ui0 == ui0_2);

	LilvUI ui2 = lilv_uis_get_by_uri(uis, ui2_uri);
	TEST_ASSERT(ui2 != ui0);

	LilvUI ui3 = lilv_uis_get_by_uri(uis, ui3_uri);
	TEST_ASSERT(ui3 != ui0);

	LilvUI noui = lilv_uis_get_by_uri(uis, noui_uri);
	TEST_ASSERT(noui == NULL);

	LilvValues classes = lilv_ui_get_classes(ui0);
	TEST_ASSERT(lilv_values_size(classes) == 1);

	LilvValue ui_class_uri = lilv_value_new_uri(world,
			"http://lv2plug.in/ns/extensions/ui#GtkUI");

	TEST_ASSERT(lilv_value_equals(lilv_values_get_first(classes), ui_class_uri));
	TEST_ASSERT(lilv_ui_is_a(ui0, ui_class_uri));

	LilvValue plug_bundle_uri = lilv_plugin_get_bundle_uri(plug);
	LilvValue ui_bundle_uri = lilv_ui_get_bundle_uri(ui0);
	TEST_ASSERT(lilv_value_equals(plug_bundle_uri, ui_bundle_uri));

	char* ui_binary_uri_str = (char*)malloc(TEST_PATH_MAX);
	snprintf(ui_binary_uri_str, TEST_PATH_MAX, "%s%s",
			lilv_value_as_string(plug_bundle_uri), "ui.so");

	LilvValue ui_binary_uri = lilv_ui_get_binary_uri(ui0);

	LilvValue expected_uri = lilv_value_new_uri(world, ui_binary_uri_str);
	TEST_ASSERT(lilv_value_equals(expected_uri, ui_binary_uri));

	free(ui_binary_uri_str);
	lilv_value_free(ui_class_uri);
	lilv_value_free(ui_uri);
	lilv_value_free(ui2_uri);
	lilv_value_free(ui3_uri);
	lilv_value_free(noui_uri);
	lilv_value_free(expected_uri);
	lilv_uis_free(uis);

	cleanup_uris();
	return 1;
}

/*****************************************************************************/

/* add tests here */
static struct TestCase tests[] = {
	TEST_CASE(utils),
	TEST_CASE(value),
	TEST_CASE(verify),
	TEST_CASE(no_verify),
	TEST_CASE(discovery),
	TEST_CASE(classes),
	TEST_CASE(plugin),
	TEST_CASE(port),
	TEST_CASE(ui),
	{ NULL, NULL }
};

void
run_tests()
{
	int i;
	for (i = 0; tests[i].title; i++) {
		printf("*** Test %s\n", tests[i].title);
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
	setenv("LANG", "C", 1);
	init_tests();
	run_tests();
	cleanup();
	printf("\n*** Test Results: %d tests, %d errors\n\n", test_count, error_count);
	return error_count ? 1 : 0;
}
