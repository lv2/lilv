/*
  Copyright 2020 David Robillard <http://drobilla.net>

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

#define _POSIX_C_SOURCE 200809L /* for setenv */

#include "lilv_test_utils.h"

#include "../src/filesystem.h"
#include "../src/lilv_internal.h"

#include "lilv/lilv.h"
#include "serd/serd.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LilvTestEnv*
lilv_test_env_new(void)
{
	LilvWorld* world = lilv_world_new();
	if (!world) {
		return NULL;
	}

	LilvTestEnv* env = (LilvTestEnv*)calloc(1, sizeof(LilvTestEnv));

	env->world       = world;
	env->plugin1_uri = lilv_new_uri(world, "http://example.org/plug");
	env->plugin2_uri = lilv_new_uri(world, "http://example.org/foobar");

	// Set custom LV2_PATH in build directory to only use test data
	char*     test_path = lilv_path_canonical(LILV_TEST_DIR);
	char*     lv2_path  = lilv_strjoin(test_path, "/test_lv2_path", NULL);
	LilvNode* path      = lilv_new_string(world, lv2_path);
	lilv_world_set_option(world, LILV_OPTION_LV2_PATH, path);
	free(lv2_path);
	free(test_path);
	lilv_node_free(path);

	return env;
}

void
lilv_test_env_free(LilvTestEnv* env)
{
	free(env->test_content_path);
	free(env->test_manifest_path);
	free(env->test_bundle_uri);
	free(env->test_bundle_path);
	lilv_node_free(env->plugin2_uri);
	lilv_node_free(env->plugin1_uri);
	lilv_world_free(env->world);
	free(env);
}

int
create_bundle(LilvTestEnv* env, const char* manifest, const char* plugin)
{
	{
		static const char* const bundle_path = "/test_lv2_path/lilv-test.lv2";

		char* const test_path = lilv_path_canonical(LILV_TEST_DIR);
		env->test_bundle_path = lilv_strjoin(test_path, bundle_path, NULL);
		lilv_free(test_path);
	}

	if (lilv_create_directories(env->test_bundle_path)) {
		fprintf(stderr,
		        "Failed to create directory '%s' (%s)\n",
		        env->test_bundle_path,
		        strerror(errno));
		return 1;
	}

	SerdNode s = serd_node_new_file_uri((const uint8_t*)env->test_bundle_path,
	                                    NULL,
	                                    NULL,
	                                    true);

	env->test_bundle_uri = lilv_strjoin((const char*)s.buf, "/", NULL);
	env->test_manifest_path =
	    lilv_strjoin(env->test_bundle_path, "/manifest.ttl", NULL);
	env->test_content_path =
	    lilv_strjoin(env->test_bundle_path, "/plugin.ttl", NULL);

	serd_node_free(&s);

	FILE* const manifest_file = fopen(env->test_manifest_path, "w");
	if (!manifest_file) {
		return 2;
	}

	FILE* const plugin_file = fopen(env->test_content_path, "w");
	if (!plugin_file) {
		fclose(manifest_file);
		return 3;
	}

	const size_t manifest_head_len = strlen(MANIFEST_PREFIXES);
	const size_t manifest_len      = strlen(manifest);
	const size_t plugin_head_len   = strlen(PLUGIN_PREFIXES);
	const size_t plugin_len        = strlen(plugin);
	const size_t n_total =
	    manifest_len + plugin_len + manifest_head_len + plugin_head_len;

	size_t n_written = 0;
	n_written += fwrite(MANIFEST_PREFIXES, 1, manifest_head_len, manifest_file);
	n_written += fwrite(manifest, 1, manifest_len, manifest_file);
	n_written += fwrite(PLUGIN_PREFIXES, 1, plugin_head_len, plugin_file);
	n_written += fwrite(plugin, 1, plugin_len, plugin_file);

	fclose(manifest_file);
	fclose(plugin_file);
	return n_written == n_total ? 0 : 4;
}

int
start_bundle(LilvTestEnv* env, const char* manifest, const char* plugin)
{
	if (create_bundle(env, manifest, plugin)) {
		return 1;
	}

	lilv_world_load_all(env->world);
	return 0;
}

void
delete_bundle(LilvTestEnv* env)
{
	if (env->test_content_path) {
		lilv_remove(env->test_content_path);
	}

	if (env->test_manifest_path) {
		lilv_remove(env->test_manifest_path);
	}

	if (env->test_bundle_path) {
		remove(env->test_bundle_path);
	}

	free(env->test_content_path);
	free(env->test_manifest_path);
	free(env->test_bundle_uri);
	free(env->test_bundle_path);

	env->test_content_path  = NULL;
	env->test_manifest_path = NULL;
	env->test_bundle_uri    = NULL;
	env->test_bundle_path   = NULL;
}

void
set_env(const char* name, const char* value)
{
#ifdef _WIN32
	// setenv on Windows does not modify the current process' environment
	const size_t len = strlen(name) + 1 + strlen(value) + 1;
	char*        str = (char*)calloc(1, len);
	snprintf(str, len, "%s=%s", name, value);
	putenv(str);
	free(str);
#else
	setenv(name, value, 1);
#endif
}
