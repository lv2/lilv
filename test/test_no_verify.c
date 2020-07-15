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

static const char* const plugin_ttl = ":plug a lv2:Plugin .\n";

int
main(void)
{
	LilvTestEnv* const env   = lilv_test_env_new();
	LilvWorld* const   world = env->world;

	if (start_bundle(env, SIMPLE_MANIFEST_TTL, plugin_ttl)) {
		return 1;
	}

	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	const LilvPlugin*  explug =
	    lilv_plugins_get_by_uri(plugins, env->plugin1_uri);

	assert(explug);
	assert(!lilv_plugin_verify(explug));

	delete_bundle(env);
	lilv_test_env_free(env);

	return 0;
}
