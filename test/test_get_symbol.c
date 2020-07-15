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

static const char* const manifest_ttl = "\
	:plug a lv2:Plugin ;\n\
	lv2:symbol \"plugsym\" ;\n\
	lv2:binary <foo" SHLIB_EXT "> ;\n\
	rdfs:seeAlso <plugin.ttl> .\n";

static const char* const plugin_ttl = "\
	:plug a lv2:Plugin ;\n\
	doap:name \"Test plugin\" ;\n\
	lv2:symbol \"plugsym\" .\n";

int
main(void)
{
	LilvTestEnv* const env   = lilv_test_env_new();
	LilvWorld* const   world = env->world;

	if (start_bundle(env, manifest_ttl, plugin_ttl)) {
		return 1;
	}

	LilvNode* plug_sym  = lilv_world_get_symbol(world, env->plugin1_uri);
	LilvNode* path      = lilv_new_uri(world, "http://example.org/foo");
	LilvNode* path_sym  = lilv_world_get_symbol(world, path);
	LilvNode* query     = lilv_new_uri(world, "http://example.org/foo?bar=baz");
	LilvNode* query_sym = lilv_world_get_symbol(world, query);
	LilvNode* frag      = lilv_new_uri(world, "http://example.org/foo#bar");
	LilvNode* frag_sym  = lilv_world_get_symbol(world, frag);
	LilvNode* queryfrag =
	    lilv_new_uri(world, "http://example.org/foo?bar=baz#quux");
	LilvNode* queryfrag_sym = lilv_world_get_symbol(world, queryfrag);
	LilvNode* nonuri        = lilv_new_int(world, 42);

	assert(lilv_world_get_symbol(world, nonuri) == NULL);
	assert(!strcmp(lilv_node_as_string(plug_sym), "plugsym"));
	assert(!strcmp(lilv_node_as_string(path_sym), "foo"));
	assert(!strcmp(lilv_node_as_string(query_sym), "bar_baz"));
	assert(!strcmp(lilv_node_as_string(frag_sym), "bar"));
	assert(!strcmp(lilv_node_as_string(queryfrag_sym), "quux"));

	lilv_node_free(nonuri);
	lilv_node_free(queryfrag_sym);
	lilv_node_free(queryfrag);
	lilv_node_free(frag_sym);
	lilv_node_free(frag);
	lilv_node_free(query_sym);
	lilv_node_free(query);
	lilv_node_free(path_sym);
	lilv_node_free(path);
	lilv_node_free(plug_sym);

	delete_bundle(env);
	lilv_test_env_free(env);

	return 0;
}
