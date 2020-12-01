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

#include "lilv_test_uri_map.h"
#include "lilv_test_utils.h"

#include "../src/filesystem.h"
#include "../src/lilv_internal.h"

#include "lilv/lilv.h"
#include "lv2/core/lv2.h"
#include "lv2/state/state.h"
#include "lv2/urid/urid.h"
#include "serd/serd.h"

#ifdef _WIN32
#	include <direct.h>
#	define mkdir(path, flags) _mkdir(path)
#else
#	include <sys/stat.h>
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	LilvTestEnv* env;
	LV2_URID     atom_Float;
	float        in;
	float        out;
	float        control;
} TestContext;

static TestContext*
test_context_new(void)
{
	TestContext* ctx = (TestContext*)calloc(1, sizeof(TestContext));

	ctx->env        = lilv_test_env_new();
	ctx->atom_Float = 0;
	ctx->in         = 1.0;
	ctx->out        = 42.0;
	ctx->control    = 1234.0;

	return ctx;
}

static void
test_context_free(TestContext* ctx)
{
	lilv_test_env_free(ctx->env);
	free(ctx);
}

static const void*
get_port_value(const char* port_symbol,
               void*       user_data,
               uint32_t*   size,
               uint32_t*   type)
{
	TestContext* ctx = (TestContext*)user_data;

	if (!strcmp(port_symbol, "input")) {
		*size = sizeof(float);
		*type = ctx->atom_Float;
		return &ctx->in;
	} else if (!strcmp(port_symbol, "output")) {
		*size = sizeof(float);
		*type = ctx->atom_Float;
		return &ctx->out;
	} else if (!strcmp(port_symbol, "control")) {
		*size = sizeof(float);
		*type = ctx->atom_Float;
		return &ctx->control;
	} else {
		fprintf(stderr,
		        "error: get_port_value for nonexistent port `%s'\n",
		        port_symbol);
		*size = *type = 0;
		return NULL;
	}
}

static void
set_port_value(const char* port_symbol,
               void*       user_data,
               const void* value,
               uint32_t    size,
               uint32_t    type)
{
	TestContext* ctx = (TestContext*)user_data;

	if (!strcmp(port_symbol, "input")) {
		ctx->in = *(const float*)value;
	} else if (!strcmp(port_symbol, "output")) {
		ctx->out = *(const float*)value;
	} else if (!strcmp(port_symbol, "control")) {
		ctx->control = *(const float*)value;
	} else {
		fprintf(stderr,
		        "error: set_port_value for nonexistent port `%s'\n",
		        port_symbol);
	}
}

static char* temp_dir = NULL;

static char*
lilv_make_path(LV2_State_Make_Path_Handle handle, const char* path)
{
	return lilv_path_join(temp_dir, path);
}

static void
lilv_free_path(LV2_State_Free_Path_Handle handle, char* path)
{
	lilv_free(path);
}

int
main(void)
{
	TestContext* const ctx   = test_context_new();
	LilvTestEnv* const env   = ctx->env;
	LilvWorld* const   world = env->world;

	LilvTestUriMap uri_map;
	lilv_test_uri_map_init(&uri_map);

	uint8_t*  abs_bundle = (uint8_t*)lilv_path_absolute(LILV_TEST_BUNDLE);
	SerdNode  bundle     = serd_node_new_file_uri(abs_bundle, 0, 0, true);
	LilvNode* bundle_uri = lilv_new_uri(world, (const char*)bundle.buf);
	LilvNode* plugin_uri =
	    lilv_new_uri(world, "http://example.org/lilv-test-plugin");
	lilv_world_load_bundle(world, bundle_uri);
	free(abs_bundle);
	serd_node_free(&bundle);

	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	const LilvPlugin*  plugin  = lilv_plugins_get_by_uri(plugins, plugin_uri);
	assert(plugin);

	LV2_URID_Map       map           = {&uri_map, map_uri};
	LV2_Feature        map_feature   = {LV2_URID_MAP_URI, &map};
	LV2_URID_Unmap     unmap         = {&uri_map, unmap_uri};
	LV2_Feature        unmap_feature = {LV2_URID_UNMAP_URI, &unmap};
	const LV2_Feature* features[]    = {&map_feature, &unmap_feature, NULL};

	ctx->atom_Float =
	    map.map(map.handle, "http://lv2plug.in/ns/ext/atom#Float");

	LilvNode*  num     = lilv_new_int(world, 5);
	LilvState* nostate = lilv_state_new_from_file(world, &map, num, "/junk");
	assert(!nostate);

	LilvInstance* instance = lilv_plugin_instantiate(plugin, 48000.0, features);
	assert(instance);
	lilv_instance_activate(instance);
	lilv_instance_connect_port(instance, 0, &ctx->in);
	lilv_instance_connect_port(instance, 1, &ctx->out);
	lilv_instance_run(instance, 1);
	assert(ctx->in == 1.0);
	assert(ctx->out == 1.0);

	temp_dir = lilv_path_canonical("temp");

	const char* scratch_dir = NULL;
	char*       copy_dir    = NULL;
	char*       link_dir    = NULL;
	char*       save_dir    = NULL;

	// Get instance state state
	LilvState* state = lilv_state_new_from_instance(plugin,
	                                                instance,
	                                                &map,
	                                                scratch_dir,
	                                                copy_dir,
	                                                link_dir,
	                                                save_dir,
	                                                get_port_value,
	                                                ctx,
	                                                0,
	                                                NULL);

	// Get another instance state
	LilvState* state2 = lilv_state_new_from_instance(plugin,
	                                                 instance,
	                                                 &map,
	                                                 scratch_dir,
	                                                 copy_dir,
	                                                 link_dir,
	                                                 save_dir,
	                                                 get_port_value,
	                                                 ctx,
	                                                 0,
	                                                 NULL);

	// Ensure they are equal
	assert(lilv_state_equals(state, state2));

	// Check that we can't delete unsaved state
	assert(lilv_state_delete(world, state));

	// Check that state has no URI
	assert(!lilv_state_get_uri(state));

	// Check that we can't save a state with no URI
	char* bad_state_str =
	    lilv_state_to_string(world, &map, &unmap, state, NULL, NULL);
	assert(!bad_state_str);

	// Check that we can't restore the NULL string (and it doesn't crash)
	LilvState* bad_state = lilv_state_new_from_string(world, &map, NULL);
	assert(!bad_state);

	// Save state to a string
	char* state1_str = lilv_state_to_string(
	    world, &map, &unmap, state, "http://example.org/state1", NULL);

	// Restore from string
	LilvState* from_str = lilv_state_new_from_string(world, &map, state1_str);

	// Ensure they are equal
	assert(lilv_state_equals(state, from_str));
	lilv_free(state1_str);

	const LilvNode* state_plugin_uri = lilv_state_get_plugin_uri(state);
	assert(lilv_node_equals(state_plugin_uri, plugin_uri));

	// Tinker with the label of the first state
	assert(lilv_state_get_label(state) == NULL);
	lilv_state_set_label(state, "Test State Old Label");
	assert(!strcmp(lilv_state_get_label(state), "Test State Old Label"));
	lilv_state_set_label(state, "Test State");
	assert(!strcmp(lilv_state_get_label(state), "Test State"));

	assert(!lilv_state_equals(state, state2)); // Label changed

	// Run and get a new instance state (which should now differ)
	lilv_instance_run(instance, 1);
	LilvState* state3 = lilv_state_new_from_instance(plugin,
	                                                 instance,
	                                                 &map,
	                                                 scratch_dir,
	                                                 copy_dir,
	                                                 link_dir,
	                                                 save_dir,
	                                                 get_port_value,
	                                                 ctx,
	                                                 0,
	                                                 NULL);
	assert(!lilv_state_equals(state2, state3)); // num_runs changed

	// Restore instance state to original state
	lilv_state_restore(state2, instance, set_port_value, ctx, 0, NULL);

	// Take a new snapshot and ensure it matches the set state
	LilvState* state4 = lilv_state_new_from_instance(plugin,
	                                                 instance,
	                                                 &map,
	                                                 scratch_dir,
	                                                 copy_dir,
	                                                 link_dir,
	                                                 save_dir,
	                                                 get_port_value,
	                                                 ctx,
	                                                 0,
	                                                 NULL);
	assert(lilv_state_equals(state2, state4));

	// Set some metadata properties
	lilv_state_set_metadata(state,
	                        map.map(map.handle, LILV_NS_RDFS "comment"),
	                        "This is a comment",
	                        strlen("This is a comment") + 1,
	                        map.map(map.handle,
	                                "http://lv2plug.in/ns/ext/atom#Literal"),
	                        LV2_STATE_IS_POD);
	lilv_state_set_metadata(state,
	                        map.map(map.handle, "http://example.org/metablob"),
	                        "LIVEBEEF",
	                        strlen("LIVEBEEF") + 1,
	                        map.map(map.handle, "http://example.org/MetaBlob"),
	                        0);

	// Save state to a directory
	int ret = lilv_state_save(
	    world, &map, &unmap, state, NULL, "state/state.lv2", "state.ttl");
	assert(!ret);

	// Load state from directory
	LilvState* state5 = lilv_state_new_from_file(world,
	                                             &map,
	                                             NULL,
	                                             "state/state.lv2/state.ttl");

	assert(lilv_state_equals(state, state5)); // Round trip accuracy
	assert(lilv_state_get_num_properties(state) == 8);

	// Attempt to save state to nowhere (error)
	ret = lilv_state_save(world, &map, &unmap, state, NULL, NULL, NULL);
	assert(ret);

	// Save another state to the same directory (update manifest)
	ret = lilv_state_save(
	    world, &map, &unmap, state, NULL, "state/state.lv2", "state2.ttl");
	assert(!ret);

	// Save state with URI to a directory
	const char* state_uri = "http://example.org/state";
	ret                   = lilv_state_save(world,
                          &map,
                          &unmap,
                          state,
                          state_uri,
                          "state/state6.lv2",
                          "state6.ttl");
	assert(!ret);

	// Load state bundle into world and verify it matches
	{
		uint8_t* state6_path =
		    (uint8_t*)lilv_path_absolute("state/state6.lv2/");
		SerdNode  state6_uri = serd_node_new_file_uri(state6_path, 0, 0, true);
		LilvNode* state6_bundle =
		    lilv_new_uri(world, (const char*)state6_uri.buf);
		LilvNode* state6_node = lilv_new_uri(world, state_uri);
		lilv_world_load_bundle(world, state6_bundle);
		lilv_world_load_resource(world, state6_node);
		serd_node_free(&state6_uri);
		lilv_free(state6_path);

		LilvState* state6 = lilv_state_new_from_world(world, &map, state6_node);
		assert(lilv_state_equals(state, state6)); // Round trip accuracy

		// Check that loaded state has correct URI
		assert(lilv_state_get_uri(state6));
		assert(!strcmp(lilv_node_as_string(lilv_state_get_uri(state6)),
		               state_uri));

		// Unload state from world
		lilv_world_unload_resource(world, state6_node);
		lilv_world_unload_bundle(world, state6_bundle);

		// Ensure that it is no longer present
		assert(!lilv_state_new_from_world(world, &map, state6_node));
		lilv_node_free(state6_bundle);
		lilv_node_free(state6_node);

		// Delete state
		lilv_state_delete(world, state6);
		lilv_state_free(state6);
	}

	// Make directories and test files support
	mkdir("temp", 0700);
	scratch_dir = temp_dir;
	mkdir("files", 0700);
	copy_dir = lilv_path_canonical("files");
	mkdir("links", 0700);
	link_dir = lilv_path_canonical("links");

	LV2_State_Make_Path make_path         = {NULL, lilv_make_path};
	LV2_Feature         make_path_feature = {LV2_STATE__makePath, &make_path};
	LV2_State_Free_Path free_path         = {NULL, lilv_free_path};
	LV2_Feature         free_path_feature = {LV2_STATE__freePath, &free_path};
	const LV2_Feature*  ffeatures[]       = {&make_path_feature,
                                      &map_feature,
                                      &free_path_feature,
                                      NULL};

	lilv_instance_deactivate(instance);
	lilv_instance_free(instance);
	instance = lilv_plugin_instantiate(plugin, 48000.0, ffeatures);
	lilv_instance_activate(instance);
	lilv_instance_connect_port(instance, 0, &ctx->in);
	lilv_instance_connect_port(instance, 1, &ctx->out);
	lilv_instance_run(instance, 1);

	// Test instantiating twice
	LilvInstance* instance2 =
	    lilv_plugin_instantiate(plugin, 48000.0, ffeatures);
	if (!instance2) {
		fprintf(stderr,
		        "Failed to create multiple instances of <%s>\n",
		        lilv_node_as_uri(state_plugin_uri));
		return 1;
	}
	lilv_instance_free(instance2);

	// Get instance state state
	LilvState* fstate = lilv_state_new_from_instance(plugin,
	                                                 instance,
	                                                 &map,
	                                                 scratch_dir,
	                                                 copy_dir,
	                                                 link_dir,
	                                                 "state/fstate.lv2",
	                                                 get_port_value,
	                                                 ctx,
	                                                 0,
	                                                 ffeatures);

	{
		// Get another instance state
		LilvState* fstate2 = lilv_state_new_from_instance(plugin,
		                                                  instance,
		                                                  &map,
		                                                  scratch_dir,
		                                                  copy_dir,
		                                                  link_dir,
		                                                  "state/fstate2.lv2",
		                                                  get_port_value,
		                                                  ctx,
		                                                  0,
		                                                  ffeatures);

		// Check that it is identical
		assert(lilv_state_equals(fstate, fstate2));

		lilv_state_delete(world, fstate2);
		lilv_state_free(fstate2);
	}

	// Run, writing more to rec file
	lilv_instance_run(instance, 2);

	// Get yet another instance state
	LilvState* fstate3 = lilv_state_new_from_instance(plugin,
	                                                  instance,
	                                                  &map,
	                                                  scratch_dir,
	                                                  copy_dir,
	                                                  link_dir,
	                                                  "state/fstate3.lv2",
	                                                  get_port_value,
	                                                  ctx,
	                                                  0,
	                                                  ffeatures);

	// Should be different
	assert(!lilv_state_equals(fstate, fstate3));

	// Save state to a directory
	ret = lilv_state_save(
	    world, &map, &unmap, fstate, NULL, "state/fstate.lv2", "fstate.ttl");
	assert(!ret);

	// Load state from directory
	LilvState* fstate4 = lilv_state_new_from_file(
	    world, &map, NULL, "state/fstate.lv2/fstate.ttl");
	assert(lilv_state_equals(fstate, fstate4)); // Round trip accuracy

	// Restore instance state to loaded state
	lilv_state_restore(fstate4, instance, set_port_value, ctx, 0, ffeatures);

	// Take a new snapshot and ensure it matches
	LilvState* fstate5 = lilv_state_new_from_instance(plugin,
	                                                  instance,
	                                                  &map,
	                                                  scratch_dir,
	                                                  copy_dir,
	                                                  link_dir,
	                                                  "state/fstate5.lv2",
	                                                  get_port_value,
	                                                  ctx,
	                                                  0,
	                                                  ffeatures);
	assert(lilv_state_equals(fstate3, fstate5));

	// Save state to a (different) directory again
	ret = lilv_state_save(
	    world, &map, &unmap, fstate, NULL, "state/fstate6.lv2", "fstate6.ttl");
	assert(!ret);

	// Reload it and ensure it's identical to the other loaded version
	LilvState* fstate6 = lilv_state_new_from_file(
	    world, &map, NULL, "state/fstate6.lv2/fstate6.ttl");
	assert(lilv_state_equals(fstate4, fstate6));

	// Run, changing rec file (without changing size)
	lilv_instance_run(instance, 3);

	// Take a new snapshot
	LilvState* fstate7 = lilv_state_new_from_instance(plugin,
	                                                  instance,
	                                                  &map,
	                                                  scratch_dir,
	                                                  copy_dir,
	                                                  link_dir,
	                                                  "state/fstate7.lv2",
	                                                  get_port_value,
	                                                  ctx,
	                                                  0,
	                                                  ffeatures);
	assert(!lilv_state_equals(fstate6, fstate7));

	// Save the changed state to a (different) directory again
	ret = lilv_state_save(
	    world, &map, &unmap, fstate7, NULL, "state/fstate7.lv2", "fstate7.ttl");
	assert(!ret);

	// Reload it and ensure it's changed
	LilvState* fstate72 = lilv_state_new_from_file(
	    world, &map, NULL, "state/fstate7.lv2/fstate7.ttl");
	assert(lilv_state_equals(fstate72, fstate7));
	assert(!lilv_state_equals(fstate6, fstate72));

	// Delete saved state we still have a state in memory that points to
	lilv_state_delete(world, fstate7);
	lilv_state_delete(world, fstate6);
	lilv_state_delete(world, fstate5);
	lilv_state_delete(world, fstate3);
	lilv_state_delete(world, fstate);
	lilv_state_delete(world, state2);
	lilv_state_delete(world, state);

	// Delete remaining states on disk we've lost a reference to
	const char* const old_state_paths[] = {"state/state.lv2/state.ttl",
	                                       "state/state.lv2/state2.ttl",
	                                       "state/fstate.lv2/fstate.ttl",
	                                       NULL};

	for (const char* const* p = old_state_paths; *p; ++p) {
		const char* path = *p;
		LilvState*  old_state =
		    lilv_state_new_from_file(world, &map, NULL, path);
		lilv_state_delete(world, old_state);
		lilv_state_free(old_state);
	}

	lilv_instance_deactivate(instance);
	lilv_instance_free(instance);

	lilv_node_free(num);

	lilv_state_free(state);
	lilv_state_free(from_str);
	lilv_state_free(state2);
	lilv_state_free(state3);
	lilv_state_free(state4);
	lilv_state_free(state5);
	lilv_state_free(fstate);
	lilv_state_free(fstate3);
	lilv_state_free(fstate4);
	lilv_state_free(fstate5);
	lilv_state_free(fstate6);
	lilv_state_free(fstate7);
	lilv_state_free(fstate72);

	lilv_remove("state");

	lilv_test_uri_map_clear(&uri_map);

	lilv_node_free(plugin_uri);
	lilv_node_free(bundle_uri);
	free(link_dir);
	free(copy_dir);
	free(temp_dir);

	test_context_free(ctx);

	return 0;
}
