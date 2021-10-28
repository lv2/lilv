#undef NDEBUG

#include "../src/filesystem.h"

#include "lilv/lilv.h"
#include "serd/serd.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define PLUGIN_URI "http://example.org/lib-descriptor"

int
main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s BUNDLE\n", argv[0]);
    return 1;
  }

  const char* bundle_path = argv[1];
  LilvWorld*  world       = lilv_world_new();

  // Load test plugin bundle
  char*     abs_bundle = lilv_path_absolute(bundle_path);
  SerdNode* bundle =
    serd_new_file_uri(NULL, SERD_STRING(abs_bundle), SERD_EMPTY_STRING());

  lilv_world_load_bundle(world, bundle);
  free(abs_bundle);
  serd_node_free(NULL, bundle);

  LilvNode*          plugin_uri = lilv_new_uri(world, PLUGIN_URI);
  const LilvPlugins* plugins    = lilv_world_get_all_plugins(world);
  const LilvPlugin*  plugin     = lilv_plugins_get_by_uri(plugins, plugin_uri);
  assert(plugin);

  LilvInstance* instance = lilv_plugin_instantiate(plugin, 48000.0, NULL);
  assert(instance);
  lilv_instance_free(instance);

  LilvNode* eg_blob = lilv_new_uri(world, "http://example.org/blob");
  LilvNode* blob    = lilv_world_get(world, plugin_uri, eg_blob, NULL);
  assert(lilv_node_is_literal(blob));
  lilv_node_free(blob);
  lilv_node_free(eg_blob);

  LilvNode* eg_junk = lilv_new_uri(world, "http://example.org/junk");
  LilvNode* junk    = lilv_world_get(world, plugin_uri, eg_junk, NULL);
  assert(lilv_node_is_literal(junk));
  lilv_node_free(junk);
  lilv_node_free(eg_junk);

  lilv_node_free(plugin_uri);
  lilv_world_free(world);

  return 0;
}
