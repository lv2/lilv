#undef NDEBUG

#include "../src/filesystem.h"

#include "lilv/lilv.h"
#include "serd/serd.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define PLUGIN_URI "http://example.org/missing-port-name"

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
    serd_new_file_uri(NULL, serd_string(abs_bundle), serd_empty_string());

  lilv_world_load_bundle(world, bundle);
  free(abs_bundle);
  serd_node_free(NULL, bundle);

  LilvNode*          plugin_uri = lilv_new_uri(world, PLUGIN_URI);
  const LilvPlugins* plugins    = lilv_world_get_all_plugins(world);
  const LilvPlugin*  plugin     = lilv_plugins_get_by_uri(plugins, plugin_uri);
  assert(plugin);

  const LilvPort* port = lilv_plugin_get_port_by_index(plugin, 0);
  assert(port);
  LilvNode* name = lilv_port_get_name(plugin, port);
  assert(!name);
  lilv_node_free(name);

  lilv_node_free(plugin_uri);
  lilv_world_free(world);

  return 0;
}
