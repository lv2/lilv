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

int
main(void)
{
	LilvTestEnv* const env   = lilv_test_env_new();
	LilvWorld* const   world = env->world;

	// Create a simple plugin bundle
	create_bundle(env,
	              ":plug a lv2:Plugin ; lv2:binary <foo" SHLIB_EXT
	              "> ; rdfs:seeAlso <plugin.ttl> .\n",
	              ":plug a lv2:Plugin ; "
	              "doap:name \"First name\" .");

	lilv_world_load_specifications(world);

	// Load bundle
	LilvNode* bundle_uri = lilv_new_uri(world, env->test_bundle_uri);
	lilv_world_load_bundle(world, bundle_uri);

	// Check that plugin is present
	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	const LilvPlugin* plug = lilv_plugins_get_by_uri(plugins, env->plugin1_uri);
	assert(plug);

	// Check that plugin name is correct
	LilvNode* name = lilv_plugin_get_name(plug);
	assert(!strcmp(lilv_node_as_string(name), "First name"));
	lilv_node_free(name);

	// Unload bundle from world and delete it
	lilv_world_unload_bundle(world, bundle_uri);
	delete_bundle(env);

	// Create a new version of the same bundle, but with a different name
	create_bundle(env,
	              ":plug a lv2:Plugin ; lv2:binary <foo" SHLIB_EXT
	              "> ; rdfs:seeAlso <plugin.ttl> .\n",
	              ":plug a lv2:Plugin ; "
	              "doap:name \"Second name\" .");

	// Check that plugin is no longer in the world's plugin list
	assert(lilv_plugins_size(plugins) == 0);

	// Load new bundle
	lilv_world_load_bundle(world, bundle_uri);

	// Check that plugin is present again and is the same LilvPlugin
	const LilvPlugin* plug2 =
	    lilv_plugins_get_by_uri(plugins, env->plugin1_uri);
	assert(plug2);
	assert(plug2 == plug);

	// Check that plugin now has new name
	LilvNode* name2 = lilv_plugin_get_name(plug2);
	assert(name2);
	assert(!strcmp(lilv_node_as_string(name2), "Second name"));
	lilv_node_free(name2);

	// Load new bundle again (noop)
	lilv_world_load_bundle(world, bundle_uri);

	lilv_node_free(bundle_uri);
	delete_bundle(env);
	lilv_test_env_free(env);

	return 0;
}
