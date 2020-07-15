/*
  Copyright 2007-2020 David Robillard <http://drobilla.net>

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

#undef NDEBUG

#include "lilv_test_utils.h"

#include "lilv/lilv.h"

#include <assert.h>
#include <string.h>

static const char* const plugin_ttl = "\
:plug a lv2:Plugin ;\n\
	a lv2:CompressorPlugin ;\n\
	doap:name \"Test plugin\" ;\n\
	lv2:port [\n\
		a lv2:ControlPort ;\n\
		a lv2:InputPort ;\n\
		lv2:index 0 ;\n\
		lv2:symbol \"foo\" ;\n\
		lv2:name \"Foo\" ;\n\
] .";

int
main(void)
{
	LilvTestEnv* const env   = lilv_test_env_new();
	LilvWorld* const   world = env->world;

	if (start_bundle(env, SIMPLE_MANIFEST_TTL, plugin_ttl)) {
		return 1;
	}

	const LilvPluginClass*   plugin   = lilv_world_get_plugin_class(world);
	const LilvPluginClasses* classes  = lilv_world_get_plugin_classes(world);
	LilvPluginClasses*       children = lilv_plugin_class_get_children(plugin);

	assert(lilv_plugin_class_get_parent_uri(plugin) == NULL);
	assert(lilv_plugin_classes_size(classes) >
	       lilv_plugin_classes_size(children));
	assert(!strcmp(lilv_node_as_string(lilv_plugin_class_get_label(plugin)),
	               "Plugin"));
	assert(!strcmp(lilv_node_as_string(lilv_plugin_class_get_uri(plugin)),
	               "http://lv2plug.in/ns/lv2core#Plugin"));

	LILV_FOREACH (plugin_classes, i, children) {
		assert(lilv_node_equals(lilv_plugin_class_get_parent_uri(
		                            lilv_plugin_classes_get(children, i)),
		                        lilv_plugin_class_get_uri(plugin)));
	}

	LilvNode* some_uri = lilv_new_uri(world, "http://example.org/whatever");
	assert(lilv_plugin_classes_get_by_uri(classes, some_uri) == NULL);
	lilv_node_free(some_uri);

	lilv_plugin_classes_free(children);
	delete_bundle(env);
	lilv_test_env_free(env);

	return 0;
}
