/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>
  Copyright 2008 Krzysztof Foltman

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

#define _POSIX_C_SOURCE 200112L /* for setenv */

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
#include "../src/lilv_internal.h"

#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#define TEST_PATH_MAX 1024

static char bundle_dir_name[TEST_PATH_MAX];
static char bundle_dir_uri[TEST_PATH_MAX];
static char manifest_name[TEST_PATH_MAX];
static char content_name[TEST_PATH_MAX];

static LilvWorld* world;

int test_count  = 0;
int error_count = 0;

void
delete_bundle(void)
{
	unlink(content_name);
	unlink(manifest_name);
	rmdir(bundle_dir_name);
}

void
init_tests(void)
{
	strncpy(bundle_dir_name, getenv("HOME"), 900);
	strcat(bundle_dir_name, "/.lv2");
	mkdir(bundle_dir_name, 0700);
	strcat(bundle_dir_name, "/lilv-test.lv2");
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
init_world(void)
{
	world = lilv_world_new();
	return world != NULL;
}

int
load_all_bundles(void)
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
unload_bundle(void)
{
	if (world)
		lilv_world_free(world);
	world = NULL;
}

void
cleanup(void)
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

typedef int (*TestFunc)(void);

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
static LilvNode* plugin_uri_value;
static LilvNode* plugin2_uri_value;

/*****************************************************************************/

void
init_uris(void)
{
	plugin_uri_value = lilv_new_uri(world, uris_plugin);
	plugin2_uri_value = lilv_new_uri(world, "http://example.org/foobar");
	TEST_ASSERT(plugin_uri_value);
	TEST_ASSERT(plugin2_uri_value);
}

void
cleanup_uris(void)
{
	lilv_node_free(plugin2_uri_value);
	lilv_node_free(plugin_uri_value);
	plugin2_uri_value = NULL;
	plugin_uri_value = NULL;
}

/*****************************************************************************/

int
test_utils(void)
{
	TEST_ASSERT(!strcmp(lilv_uri_to_path("file:///tmp/blah"), "/tmp/blah"));
	TEST_ASSERT(!lilv_uri_to_path("file:/example.org/blah"));
	TEST_ASSERT(!lilv_uri_to_path("http://example.org/blah"));
	return 1;
}

/*****************************************************************************/

int
test_value(void)
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

	LilvNode* uval = lilv_new_uri(world, "http://example.org");
	LilvNode* sval = lilv_new_string(world, "Foo");
	LilvNode* ival = lilv_new_int(world, 42);
	LilvNode* fval = lilv_new_float(world, 1.6180);

	TEST_ASSERT(lilv_node_is_uri(uval));
	TEST_ASSERT(lilv_node_is_string(sval));
	TEST_ASSERT(lilv_node_is_int(ival));
	TEST_ASSERT(lilv_node_is_float(fval));

	TEST_ASSERT(!lilv_node_is_literal(uval));
	TEST_ASSERT(lilv_node_is_literal(sval));
	TEST_ASSERT(lilv_node_is_literal(ival));
	TEST_ASSERT(lilv_node_is_literal(fval));

	TEST_ASSERT(!strcmp(lilv_node_as_uri(uval), "http://example.org"));
	TEST_ASSERT(!strcmp(lilv_node_as_string(sval), "Foo"));
	TEST_ASSERT(lilv_node_as_int(ival) == 42);
	TEST_ASSERT(fabs(lilv_node_as_float(fval) - 1.6180) < FLT_EPSILON);

	char* tok = lilv_node_get_turtle_token(uval);
	TEST_ASSERT(!strcmp(tok, "<http://example.org>"));
	free(tok);
	tok = lilv_node_get_turtle_token(sval);
	TEST_ASSERT(!strcmp(tok, "Foo"));
	free(tok);
	tok = lilv_node_get_turtle_token(ival);
	TEST_ASSERT(!strcmp(tok, "42"));
	free(tok);
	tok = lilv_node_get_turtle_token(fval);
	TEST_ASSERT(!strncmp(tok, "1.6180", 6));
	free(tok);

	LilvNode* uval_e = lilv_new_uri(world, "http://example.org");
	LilvNode* sval_e = lilv_new_string(world, "Foo");
	LilvNode* ival_e = lilv_new_int(world, 42);
	LilvNode* fval_e = lilv_new_float(world, 1.6180);
	LilvNode* uval_ne = lilv_new_uri(world, "http://no-example.org");
	LilvNode* sval_ne = lilv_new_string(world, "Bar");
	LilvNode* ival_ne = lilv_new_int(world, 24);
	LilvNode* fval_ne = lilv_new_float(world, 3.14159);

	TEST_ASSERT(lilv_node_equals(uval, uval_e));
	TEST_ASSERT(lilv_node_equals(sval, sval_e));
	TEST_ASSERT(lilv_node_equals(ival, ival_e));
	TEST_ASSERT(lilv_node_equals(fval, fval_e));

	TEST_ASSERT(!lilv_node_equals(uval, uval_ne));
	TEST_ASSERT(!lilv_node_equals(sval, sval_ne));
	TEST_ASSERT(!lilv_node_equals(ival, ival_ne));
	TEST_ASSERT(!lilv_node_equals(fval, fval_ne));

	TEST_ASSERT(!lilv_node_equals(uval, sval));
	TEST_ASSERT(!lilv_node_equals(sval, ival));
	TEST_ASSERT(!lilv_node_equals(ival, fval));

	LilvNode* uval_dup = lilv_node_duplicate(uval);
	TEST_ASSERT(lilv_node_equals(uval, uval_dup));

	LilvNode* ifval = lilv_new_float(world, 42.0);
	TEST_ASSERT(!lilv_node_equals(ival, ifval));
	lilv_node_free(ifval);

	LilvNode* nil = NULL;
	TEST_ASSERT(!lilv_node_equals(uval, nil));
	TEST_ASSERT(!lilv_node_equals(nil, uval));
	TEST_ASSERT(lilv_node_equals(nil, nil));

	LilvNode* nil2 = lilv_node_duplicate(nil);
	TEST_ASSERT(lilv_node_equals(nil, nil2));

	lilv_node_free(uval);
	lilv_node_free(sval);
	lilv_node_free(ival);
	lilv_node_free(fval);
	lilv_node_free(uval_e);
	lilv_node_free(sval_e);
	lilv_node_free(ival_e);
	lilv_node_free(fval_e);
	lilv_node_free(uval_ne);
	lilv_node_free(sval_ne);
	lilv_node_free(ival_ne);
	lilv_node_free(fval_ne);
	lilv_node_free(uval_dup);
	lilv_node_free(nil2);

	cleanup_uris();
	return 1;
}

/*****************************************************************************/

static int discovery_plugin_found = 0;

static void
discovery_verify_plugin(const LilvPlugin* plugin)
{
	const LilvNode* value = lilv_plugin_get_uri(plugin);
	if (lilv_node_equals(value, plugin_uri_value)) {
		const LilvNode* lib_uri = NULL;
		TEST_ASSERT(!lilv_node_equals(value, plugin2_uri_value));
		discovery_plugin_found = 1;
		lib_uri = lilv_plugin_get_library_uri(plugin);
		TEST_ASSERT(lib_uri);
		TEST_ASSERT(lilv_node_is_uri(lib_uri));
		TEST_ASSERT(lilv_node_as_uri(lib_uri));
		TEST_ASSERT(strstr(lilv_node_as_uri(lib_uri), "foo.so"));
		TEST_ASSERT(lilv_plugin_verify(plugin));
	}
}

int
test_discovery(void)
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

	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	TEST_ASSERT(lilv_plugins_size(plugins) > 0);

	const const LilvPlugin* explug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(explug != NULL);
	const const LilvPlugin* explug2 = lilv_plugins_get_by_uri(plugins, plugin2_uri_value);
	TEST_ASSERT(explug2 == NULL);

	if (explug) {
		LilvNode* name = lilv_plugin_get_name(explug);
		TEST_ASSERT(!strcmp(lilv_node_as_string(name), "Test plugin"));
		lilv_node_free(name);
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
test_verify(void)
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
	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	const const LilvPlugin* explug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(explug);
	TEST_ASSERT(lilv_plugin_verify(explug));
	cleanup_uris();
	return 1;
}

/*****************************************************************************/

int
test_no_verify(void)
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin . "))
		return 0;

	init_uris();
	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	const LilvPlugin* explug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(explug);
	TEST_ASSERT(!lilv_plugin_verify(explug));
	cleanup_uris();
	return 1;
}

/*****************************************************************************/

int
test_classes(void)
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
	const LilvPluginClass*   plugin   = lilv_world_get_plugin_class(world);
	const LilvPluginClasses* classes  = lilv_world_get_plugin_classes(world);
	LilvPluginClasses*       children = lilv_plugin_class_get_children(plugin);

	TEST_ASSERT(lilv_plugin_class_get_parent_uri(plugin) == NULL);
	TEST_ASSERT(lilv_plugin_classes_size(classes) > lilv_plugin_classes_size(children));
	TEST_ASSERT(!strcmp(lilv_node_as_string(lilv_plugin_class_get_label(plugin)), "Plugin"));
	TEST_ASSERT(!strcmp(lilv_node_as_string(lilv_plugin_class_get_uri(plugin)),
			"http://lv2plug.in/ns/lv2core#Plugin"));

	LILV_FOREACH(plugin_classes, i, children) {
		TEST_ASSERT(lilv_node_equals(
				lilv_plugin_class_get_parent_uri(lilv_plugin_classes_get(children, i)),
				lilv_plugin_class_get_uri(plugin)));
	}

	LilvNode* some_uri = lilv_new_uri(world, "http://example.org/whatever");
	TEST_ASSERT(lilv_plugin_classes_get_by_uri(classes, some_uri) == NULL);
	lilv_node_free(some_uri);

	lilv_plugin_classes_free(children);

	cleanup_uris();
	return 1;
}

/*****************************************************************************/

int
test_plugin(void)
{
	if (!start_bundle(MANIFEST_PREFIXES
			":plug a lv2:Plugin ; lv2:binary <foo.so> ; rdfs:seeAlso <plugin.ttl> .\n",
			BUNDLE_PREFIXES
			":plug a lv2:Plugin ; a lv2:CompressorPlugin ; "
			PLUGIN_NAME("Test plugin") " ; "
			LICENSE_GPL " ; "
			"lv2:optionalFeature lv2:hardRTCapable ; "
			"lv2:requiredFeature <http://lv2plug.in/ns/ext/event> ; "
			"lv2:extensionData <http://example.org/extdata> ;"
			":foo 1.6180 ; "
			":bar true ; "
			":baz false ; "
			":blank [ a <http://example.org/blank> ] ; "
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
	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	const LilvPlugin* plug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(plug);

	const LilvPluginClass* class = lilv_plugin_get_class(plug);
	const LilvNode* class_uri = lilv_plugin_class_get_uri(class);
	TEST_ASSERT(!strcmp(lilv_node_as_string(class_uri),
			"http://lv2plug.in/ns/lv2core#CompressorPlugin"));

	const LilvNode* plug_bundle_uri = lilv_plugin_get_bundle_uri(plug);
	TEST_ASSERT(!strcmp(lilv_node_as_string(plug_bundle_uri), bundle_dir_uri));

	const LilvNodes* data_uris = lilv_plugin_get_data_uris(plug);
	TEST_ASSERT(lilv_nodes_size(data_uris) == 2);

	char* manifest_uri = (char*)malloc(TEST_PATH_MAX);
	char* data_uri     = (char*)malloc(TEST_PATH_MAX);
	snprintf(manifest_uri, TEST_PATH_MAX, "%s%s",
			lilv_node_as_string(plug_bundle_uri), "manifest.ttl");
	snprintf(data_uri, TEST_PATH_MAX, "%s%s",
			lilv_node_as_string(plug_bundle_uri), "plugin.ttl");

	LilvNode* manifest_uri_val = lilv_new_uri(world, manifest_uri);
	TEST_ASSERT(lilv_nodes_contains(data_uris, manifest_uri_val));
	lilv_node_free(manifest_uri_val);

	LilvNode* data_uri_val = lilv_new_uri(world, data_uri);
	TEST_ASSERT(lilv_nodes_contains(data_uris, data_uri_val));
	lilv_node_free(data_uri_val);

	free(manifest_uri);
	free(data_uri);

	float mins[1];
	float maxs[1];
	float defs[1];
	lilv_plugin_get_port_ranges_float(plug, mins, maxs, defs);
	TEST_ASSERT(mins[0] == -1.0f);
	TEST_ASSERT(maxs[0] == 1.0f);
	TEST_ASSERT(defs[0] == 0.5f);

	LilvNode* audio_class = lilv_new_uri(world,
			"http://lv2plug.in/ns/lv2core#AudioPort");
	LilvNode* control_class = lilv_new_uri(world,
			"http://lv2plug.in/ns/lv2core#ControlPort");
	LilvNode* in_class = lilv_new_uri(world,
			"http://lv2plug.in/ns/lv2core#InputPort");
	LilvNode* out_class = lilv_new_uri(world,
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

	LilvNode* rt_feature = lilv_new_uri(world,
			"http://lv2plug.in/ns/lv2core#hardRTCapable");
	LilvNode* event_feature = lilv_new_uri(world,
			"http://lv2plug.in/ns/ext/event");
	LilvNode* pretend_feature = lilv_new_uri(world,
			"http://example.org/solvesWorldHunger");

	TEST_ASSERT(lilv_plugin_has_feature(plug, rt_feature));
	TEST_ASSERT(lilv_plugin_has_feature(plug, event_feature));
	TEST_ASSERT(!lilv_plugin_has_feature(plug, pretend_feature));

	lilv_node_free(rt_feature);
	lilv_node_free(event_feature);
	lilv_node_free(pretend_feature);

	LilvNodes* supported = lilv_plugin_get_supported_features(plug);
	LilvNodes* required = lilv_plugin_get_required_features(plug);
	LilvNodes* optional = lilv_plugin_get_optional_features(plug);
	TEST_ASSERT(lilv_nodes_size(supported) == 2);
	TEST_ASSERT(lilv_nodes_size(required) == 1);
	TEST_ASSERT(lilv_nodes_size(optional) == 1);
	lilv_nodes_free(supported);
	lilv_nodes_free(required);
	lilv_nodes_free(optional);

	LilvNode*  foo_p = lilv_new_uri(world, "http://example.org/foo");
	LilvNodes* foos  = lilv_plugin_get_value(plug, foo_p);
	TEST_ASSERT(lilv_nodes_size(foos) == 1);
	TEST_ASSERT(fabs(lilv_node_as_float(lilv_nodes_get_first(foos)) - 1.6180) < FLT_EPSILON);
	lilv_node_free(foo_p);
	lilv_nodes_free(foos);

	LilvNode*  bar_p = lilv_new_uri(world, "http://example.org/bar");
	LilvNodes* bars  = lilv_plugin_get_value(plug, bar_p);
	TEST_ASSERT(lilv_nodes_size(bars) == 1);
	TEST_ASSERT(lilv_node_as_bool(lilv_nodes_get_first(bars)) == true);
	lilv_node_free(bar_p);
	lilv_nodes_free(bars);

	LilvNode*  baz_p = lilv_new_uri(world, "http://example.org/baz");
	LilvNodes* bazs  = lilv_plugin_get_value(plug, baz_p);
	TEST_ASSERT(lilv_nodes_size(bazs) == 1);
	TEST_ASSERT(lilv_node_as_bool(lilv_nodes_get_first(bazs)) == false);
	lilv_node_free(baz_p);
	lilv_nodes_free(bazs);

	LilvNode*  blank_p = lilv_new_uri(world, "http://example.org/blank");
	LilvNodes* blanks  = lilv_plugin_get_value(plug, blank_p);
	TEST_ASSERT(lilv_nodes_size(blanks) == 1);
	LilvNode*  blank = lilv_nodes_get_first(blanks);
	TEST_ASSERT(lilv_node_is_blank(blank));
	const char* blank_str = lilv_node_as_blank(blank);
	char*       blank_tok = lilv_node_get_turtle_token(blank);
	TEST_ASSERT(!strncmp(blank_tok, "_:", 2));
	TEST_ASSERT(!strcmp(blank_tok + 2, blank_str));
	free(blank_tok);
	lilv_node_free(blank_p);
	lilv_nodes_free(blanks);

	LilvNode* author_name = lilv_plugin_get_author_name(plug);
	TEST_ASSERT(!strcmp(lilv_node_as_string(author_name), "David Robillard"));
	lilv_node_free(author_name);

	LilvNode* author_email = lilv_plugin_get_author_email(plug);
	TEST_ASSERT(!strcmp(lilv_node_as_string(author_email), "mailto:d@drobilla.net"));
	lilv_node_free(author_email);

	LilvNode* author_homepage = lilv_plugin_get_author_homepage(plug);
	TEST_ASSERT(!strcmp(lilv_node_as_string(author_homepage), "http://drobilla.net"));
	lilv_node_free(author_homepage);

	LilvNode* thing_uri = lilv_new_uri(world, "http://example.org/thing");
	LilvNode* name_p = lilv_new_uri(world, "http://usefulinc.com/ns/doap#name");
	LilvNodes* thing_names = lilv_world_find_nodes(world, thing_uri, name_p, NULL);
	TEST_ASSERT(lilv_nodes_size(thing_names) == 1);
	LilvNode* thing_name = lilv_nodes_get_first(thing_names);
	TEST_ASSERT(thing_name);
	TEST_ASSERT(lilv_node_is_string(thing_name));
	TEST_ASSERT(!strcmp(lilv_node_as_string(thing_name), "Something else"));

	LilvUIs* uis = lilv_plugin_get_uis(plug);
	TEST_ASSERT(lilv_uis_size(uis) == 0);
	lilv_uis_free(uis);

	LilvNode* extdata = lilv_new_uri(world, "http://example.org/extdata");
	LilvNode* noextdata = lilv_new_uri(world, "http://example.org/noextdata");
	LilvNodes* extdatas = lilv_plugin_get_extension_data(plug);
	TEST_ASSERT(lilv_plugin_has_extension_data(plug, extdata));
	TEST_ASSERT(!lilv_plugin_has_extension_data(plug, noextdata));
	TEST_ASSERT(lilv_nodes_size(extdatas) == 1);
	TEST_ASSERT(lilv_node_equals(lilv_nodes_get_first(extdatas), extdata));
	lilv_node_free(noextdata);
	lilv_node_free(extdata);
	lilv_nodes_free(extdatas);

	lilv_nodes_free(thing_names);
	lilv_node_free(thing_uri);
	lilv_node_free(name_p);
	lilv_node_free(control_class);
	lilv_node_free(audio_class);
	lilv_node_free(in_class);
	lilv_node_free(out_class);
	cleanup_uris();
	return 1;
}

/*****************************************************************************/

int
test_port(void)
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
			"  rdfs:comment \"comment\"@en , \"commentaires\"@fr ; "
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
	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	const LilvPlugin*  plug    = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(plug);

	LilvNode*       psym = lilv_new_string(world, "foo");
	const LilvPort* p    = lilv_plugin_get_port_by_index(plug, 0);
	const LilvPort* p2   = lilv_plugin_get_port_by_symbol(plug, psym);
	lilv_node_free(psym);
	TEST_ASSERT(p != NULL);
	TEST_ASSERT(p2 != NULL);
	TEST_ASSERT(p == p2);

	LilvNode*       nopsym = lilv_new_string(world, "thisaintnoportfoo");
	const LilvPort* p3     = lilv_plugin_get_port_by_symbol(plug, nopsym);
	TEST_ASSERT(p3 == NULL);
	lilv_node_free(nopsym);

	LilvNode* audio_class = lilv_new_uri(world,
			"http://lv2plug.in/ns/lv2core#AudioPort");
	LilvNode* control_class = lilv_new_uri(world,
			"http://lv2plug.in/ns/lv2core#ControlPort");
	LilvNode* in_class = lilv_new_uri(world,
			"http://lv2plug.in/ns/lv2core#InputPort");

	TEST_ASSERT(lilv_nodes_size(lilv_port_get_classes(plug, p)) == 2);
	TEST_ASSERT(lilv_plugin_get_num_ports(plug) == 2);
	TEST_ASSERT(lilv_port_is_a(plug, p, control_class));
	TEST_ASSERT(lilv_port_is_a(plug, p, in_class));
	TEST_ASSERT(!lilv_port_is_a(plug, p, audio_class));

	LilvNodes* port_properties = lilv_port_get_properties(plug, p);
	TEST_ASSERT(lilv_nodes_size(port_properties) == 1);
	lilv_nodes_free(port_properties);

	// Untranslated name (current locale is set to "C" in main)
	TEST_ASSERT(!strcmp(lilv_node_as_string(lilv_port_get_symbol(plug, p)), "foo"));
	LilvNode* name = lilv_port_get_name(plug, p);
	TEST_ASSERT(!strcmp(lilv_node_as_string(name), "store"));
	lilv_node_free(name);

	// Exact language match
	setenv("LANG", "fr_FR", 1);
	name = lilv_port_get_name(plug, p);
	TEST_ASSERT(!strcmp(lilv_node_as_string(name), "épicerie"));
	lilv_node_free(name);

	// Exact language match (with charset suffix)
	setenv("LANG", "fr_CA.utf8", 1);
	name = lilv_port_get_name(plug, p);
	TEST_ASSERT(!strcmp(lilv_node_as_string(name), "dépanneur"));
	lilv_node_free(name);

	// Partial language match (choose value translated for different country)
	setenv("LANG", "fr_BE", 1);
	name = lilv_port_get_name(plug, p);
	TEST_ASSERT((!strcmp(lilv_node_as_string(name), "dépanneur"))
	            ||(!strcmp(lilv_node_as_string(name), "épicerie")));
	lilv_node_free(name);

	// Partial language match (choose country-less language tagged value)
	setenv("LANG", "es_MX", 1);
	name = lilv_port_get_name(plug, p);
	TEST_ASSERT(!strcmp(lilv_node_as_string(name), "tienda"));
	lilv_node_free(name);

	// No language match (choose untranslated value)
	setenv("LANG", "cn", 1);
	name = lilv_port_get_name(plug, p);
	TEST_ASSERT(!strcmp(lilv_node_as_string(name), "store"));
	lilv_node_free(name);

	setenv("LANG", "en_CA.utf-8", 1);

	// Language tagged value with no untranslated values
	LilvNode*  rdfs_comment = lilv_new_uri(world, LILV_NS_RDFS "comment");
	LilvNodes* comments     = lilv_port_get_value(plug, p, rdfs_comment);
	TEST_ASSERT(!strcmp(lilv_node_as_string(lilv_nodes_get_first(comments)),
	                    "comment"));
	lilv_nodes_free(comments);

	setenv("LANG", "fr", 1);

	comments = lilv_port_get_value(plug, p, rdfs_comment);
	TEST_ASSERT(!strcmp(lilv_node_as_string(lilv_nodes_get_first(comments)),
	                    "commentaires"));
	lilv_nodes_free(comments);

	setenv("LANG", "cn", 1);

	comments = lilv_port_get_value(plug, p, rdfs_comment);
	TEST_ASSERT(!comments);
	lilv_nodes_free(comments);

	lilv_node_free(rdfs_comment);

	setenv("LANG", "C", 1);  // Reset locale
	
	LilvScalePoints* points = lilv_port_get_scale_points(plug, p);
	TEST_ASSERT(lilv_scale_points_size(points) == 2);

	LilvIter* sp_iter = lilv_scale_points_begin(points);
	const LilvScalePoint* sp0 = lilv_scale_points_get(points, sp_iter);
	TEST_ASSERT(sp0);
	sp_iter = lilv_scale_points_next(points, sp_iter);
	const LilvScalePoint* sp1 = lilv_scale_points_get(points, sp_iter);
	TEST_ASSERT(sp1);

	TEST_ASSERT(
		((!strcmp(lilv_node_as_string(lilv_scale_point_get_label(sp0)), "Sin")
		  && lilv_node_as_float(lilv_scale_point_get_value(sp0)) == 3)
		 &&
		 (!strcmp(lilv_node_as_string(lilv_scale_point_get_label(sp1)), "Cos")
		  && lilv_node_as_float(lilv_scale_point_get_value(sp1)) == 4))
		||
		((!strcmp(lilv_node_as_string(lilv_scale_point_get_label(sp0)), "Cos")
		  && lilv_node_as_float(lilv_scale_point_get_value(sp0)) == 4)
		 &&
		 (!strcmp(lilv_node_as_string(lilv_scale_point_get_label(sp1)), "Sin")
		  && lilv_node_as_float(lilv_scale_point_get_value(sp1)) == 3)));

	LilvNode* homepage_p = lilv_new_uri(world, "http://usefulinc.com/ns/doap#homepage");
	LilvNodes* homepages = lilv_plugin_get_value(plug, homepage_p);
	TEST_ASSERT(lilv_nodes_size(homepages) == 1);
	TEST_ASSERT(!strcmp(lilv_node_as_string(lilv_nodes_get_first(homepages)),
			"http://example.org/someplug"));

	LilvNode *min, *max, *def;
	lilv_port_get_range(plug, p, &def, &min, &max);
	TEST_ASSERT(def);
	TEST_ASSERT(min);
	TEST_ASSERT(max);
	TEST_ASSERT(lilv_node_as_float(def) == 0.5);
	TEST_ASSERT(lilv_node_as_float(min) == -1.0);
	TEST_ASSERT(lilv_node_as_float(max) == 1.0);

	LilvNode* integer_prop = lilv_new_uri(world, "http://lv2plug.in/ns/lv2core#integer");
	LilvNode* toggled_prop = lilv_new_uri(world, "http://lv2plug.in/ns/lv2core#toggled");

	TEST_ASSERT(lilv_port_has_property(plug, p, integer_prop));
	TEST_ASSERT(!lilv_port_has_property(plug, p, toggled_prop));

	const LilvPort* ep = lilv_plugin_get_port_by_index(plug, 1);

	LilvNode* event_type = lilv_new_uri(world, "http://example.org/event");
	LilvNode* event_type_2 = lilv_new_uri(world, "http://example.org/otherEvent");
	TEST_ASSERT(lilv_port_supports_event(plug, ep, event_type));
	TEST_ASSERT(!lilv_port_supports_event(plug, ep, event_type_2));

	LilvNode* name_p = lilv_new_uri(world, "http://lv2plug.in/ns/lv2core#name");
	LilvNodes* names = lilv_port_get_value(plug, p, name_p);
	TEST_ASSERT(lilv_nodes_size(names) == 1);
	TEST_ASSERT(!strcmp(lilv_node_as_string(lilv_nodes_get_first(names)),
	                    "store"));
	lilv_nodes_free(names);

	LilvNode* true_val  = lilv_new_bool(world, true);
	LilvNode* false_val = lilv_new_bool(world, false);

	TEST_ASSERT(!lilv_node_equals(true_val, false_val));

	lilv_world_set_option(world, LILV_OPTION_FILTER_LANG, false_val);
	names = lilv_port_get_value(plug, p, name_p);
	TEST_ASSERT(lilv_nodes_size(names) == 4);
	lilv_nodes_free(names);
	lilv_world_set_option(world, LILV_OPTION_FILTER_LANG, true_val);

	lilv_node_free(false_val);
	lilv_node_free(true_val);

	names = lilv_port_get_value(plug, ep, name_p);
	TEST_ASSERT(lilv_nodes_size(names) == 1);
	TEST_ASSERT(!strcmp(lilv_node_as_string(lilv_nodes_get_first(names)),
	                    "Event Input"));
	lilv_nodes_free(names);
	lilv_node_free(name_p);

	lilv_node_free(integer_prop);
	lilv_node_free(toggled_prop);
	lilv_node_free(event_type);
	lilv_node_free(event_type_2);

	lilv_node_free(min);
	lilv_node_free(max);
	lilv_node_free(def);

	lilv_node_free(homepage_p);
	lilv_nodes_free(homepages);

	lilv_scale_points_free(points);
	lilv_node_free(control_class);
	lilv_node_free(audio_class);
	lilv_node_free(in_class);
	cleanup_uris();
	return 1;
}

/*****************************************************************************/

static unsigned
ui_supported(const char* container_type_uri,
             const char* ui_type_uri)
{
	return !strcmp(container_type_uri, ui_type_uri);
}

int
test_ui(void)
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
	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	const LilvPlugin* plug = lilv_plugins_get_by_uri(plugins, plugin_uri_value);
	TEST_ASSERT(plug);

	LilvUIs* uis = lilv_plugin_get_uis(plug);
	TEST_ASSERT(lilv_uis_size(uis) == 4);

	const LilvUI* ui0 = lilv_uis_get(uis, lilv_uis_begin(uis));
	TEST_ASSERT(ui0);

	LilvNode* ui_uri = lilv_new_uri(world, "http://example.org/ui");
	LilvNode* ui2_uri = lilv_new_uri(world, "http://example.org/ui3");
	LilvNode* ui3_uri = lilv_new_uri(world, "http://example.org/ui4");
	LilvNode* noui_uri = lilv_new_uri(world, "http://example.org/notaui");

	const LilvUI* ui0_2 = lilv_uis_get_by_uri(uis, ui_uri);
	TEST_ASSERT(ui0 == ui0_2);
	TEST_ASSERT(lilv_node_equals(lilv_ui_get_uri(ui0_2), ui_uri));

	const LilvUI* ui2 = lilv_uis_get_by_uri(uis, ui2_uri);
	TEST_ASSERT(ui2 != ui0);

	const LilvUI* ui3 = lilv_uis_get_by_uri(uis, ui3_uri);
	TEST_ASSERT(ui3 != ui0);

	const LilvUI* noui = lilv_uis_get_by_uri(uis, noui_uri);
	TEST_ASSERT(noui == NULL);

	const LilvNodes* classes = lilv_ui_get_classes(ui0);
	TEST_ASSERT(lilv_nodes_size(classes) == 1);

	LilvNode* ui_class_uri = lilv_new_uri(world,
			"http://lv2plug.in/ns/extensions/ui#GtkUI");

	TEST_ASSERT(lilv_node_equals(lilv_nodes_get_first(classes), ui_class_uri));
	TEST_ASSERT(lilv_ui_is_a(ui0, ui_class_uri));

	const LilvNode* ui_type = NULL;
	TEST_ASSERT(lilv_ui_is_supported(ui0, ui_supported, ui_class_uri, &ui_type));
	TEST_ASSERT(lilv_node_equals(ui_type, ui_class_uri));

	const LilvNode* plug_bundle_uri = lilv_plugin_get_bundle_uri(plug);
	const LilvNode* ui_bundle_uri   = lilv_ui_get_bundle_uri(ui0);
	TEST_ASSERT(lilv_node_equals(plug_bundle_uri, ui_bundle_uri));

	char* ui_binary_uri_str = (char*)malloc(TEST_PATH_MAX);
	snprintf(ui_binary_uri_str, TEST_PATH_MAX, "%s%s",
			lilv_node_as_string(plug_bundle_uri), "ui.so");

	const LilvNode* ui_binary_uri = lilv_ui_get_binary_uri(ui0);

	LilvNode* expected_uri = lilv_new_uri(world, ui_binary_uri_str);
	TEST_ASSERT(lilv_node_equals(expected_uri, ui_binary_uri));

	free(ui_binary_uri_str);
	lilv_node_free(ui_class_uri);
	lilv_node_free(ui_uri);
	lilv_node_free(ui2_uri);
	lilv_node_free(ui3_uri);
	lilv_node_free(noui_uri);
	lilv_node_free(expected_uri);
	lilv_uis_free(uis);

	cleanup_uris();
	return 1;
}

/*****************************************************************************/

float in  = 1.0;
float out = 42.0;

LilvNode*
get_port_value(const char* port_symbol, void* user_data)
{
	if (!strcmp(port_symbol, "input")) {
		return lilv_new_float((LilvWorld*)user_data, in);
	} else if (!strcmp(port_symbol, "output")) {
		return lilv_new_float((LilvWorld*)user_data, out);
	} else {
		fprintf(stderr, "error: get_port_value for nonexistent port `%s'\n",
		        port_symbol);
		return NULL;
	}
}

void
set_port_value(const char*     port_symbol,
               const LilvNode* value,
               void*           user_data)
{
	if (!strcmp(port_symbol, "input")) {
		in = lilv_node_as_float(value);
	} else if (!strcmp(port_symbol, "output")) {
		out = lilv_node_as_float(value);
	} else {
		fprintf(stderr, "error: set_port_value for nonexistent port `%s'\n",
		        port_symbol);
	}
}

char** uris   = NULL;
size_t n_uris = 0;

LV2_URID
map_uri(LV2_URID_Map_Handle handle,
        const char*         uri)
{
	for (size_t i = 0; i < n_uris; ++i) {
		if (!strcmp(uris[i], uri)) {
			return i + 1;
		}
	}

	uris = realloc(uris, ++n_uris * sizeof(char*));
	uris[n_uris - 1] = lilv_strdup(uri);
	return n_uris;
}

const char*
unmap_uri(LV2_URID_Map_Handle handle,
          LV2_URID            urid)
{
	if (urid > 0 && urid <= n_uris) {
		return uris[urid - 1];
	}
	return NULL;
}

int
test_state(void)
{
	LilvWorld* world      = lilv_world_new();
	LilvNode*  bundle_uri = lilv_new_uri(world, LILV_TEST_BUNDLE);
	LilvNode*  plugin_uri = lilv_new_uri(world,
	                                     "http://example.org/lilv-test-plugin");
	lilv_world_load_bundle(world, bundle_uri);

	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	const LilvPlugin*  plugin  = lilv_plugins_get_by_uri(plugins, plugin_uri);
	TEST_ASSERT(plugin);

	LV2_URID_Map       map           = { NULL, map_uri };
	LV2_Feature        map_feature   = { LV2_URID_MAP_URI, &map };
	LV2_URID_Unmap     unmap         = { NULL, unmap_uri };
	LV2_Feature        unmap_feature = { LV2_URID_UNMAP_URI, &unmap };
	const LV2_Feature* features[]    = { &map_feature, &unmap_feature, NULL };

	LilvInstance* instance = lilv_plugin_instantiate(plugin, 48000.0, features);
	TEST_ASSERT(instance);

	lilv_instance_connect_port(instance, 0, &in);
	lilv_instance_connect_port(instance, 1, &out);

	lilv_instance_run(instance, 1);
	TEST_ASSERT(in == 1.0);
	TEST_ASSERT(out == 1.0);

	// Get instance state state
	LilvState* state = lilv_state_new_from_instance(
		plugin, instance, get_port_value, world, 0, NULL);

	// Get another instance state
	LilvState* state2 = lilv_state_new_from_instance(
		plugin, instance, get_port_value, world, 0, NULL);

	// Ensure they are equal
	TEST_ASSERT(lilv_state_equals(state, state2));

	const LilvNode* state_plugin_uri = lilv_state_get_plugin_uri(state);
	TEST_ASSERT(lilv_node_equals(state_plugin_uri, plugin_uri));

	// Tinker with the label of the first state
	TEST_ASSERT(lilv_state_get_label(state) == NULL);
	lilv_state_set_label(state, "Test State Old Label");
	TEST_ASSERT(!strcmp(lilv_state_get_label(state), "Test State Old Label"));
	lilv_state_set_label(state, "Test State");
	TEST_ASSERT(!strcmp(lilv_state_get_label(state), "Test State"));

	TEST_ASSERT(!lilv_state_equals(state, state2));  // Label changed

	// Run and get a new instance state (which should now differ)
	lilv_instance_run(instance, 1);
	LilvState* state3 = lilv_state_new_from_instance(
		plugin, instance, get_port_value, world, 0, NULL);
	TEST_ASSERT(!lilv_state_equals(state2, state3));  // num_runs changed

	// Restore instance state to original state
	lilv_state_restore(state2, instance, set_port_value, NULL, 0, NULL);

	// Take a new snapshot and ensure it matches the set state
	LilvState* state4 = lilv_state_new_from_instance(
		plugin, instance, get_port_value, world, 0, NULL);
	TEST_ASSERT(lilv_state_equals(state2, state4));

	// Save state to a file
	int ret = lilv_state_save(world, &unmap, state, NULL,
	                          "state.ttl", "manifest.ttl");
	TEST_ASSERT(!ret);

	// Load state from file
	LilvState* state5 = lilv_state_new_from_file(world, &map, NULL, "state.ttl");
	TEST_ASSERT(lilv_state_equals(state, state5));  // Round trip accuracy
	
	lilv_state_free(state);
	lilv_state_free(state2);
	lilv_state_free(state3);
	lilv_state_free(state4);
	lilv_state_free(state5);

	lilv_instance_free(instance);

	lilv_node_free(bundle_uri);
	lilv_world_free(world);
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
	TEST_CASE(state),
	{ NULL, NULL }
};

void
run_tests(void)
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
