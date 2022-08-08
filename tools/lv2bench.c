// Copyright 2012-2019 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "bench.h"
#include "uri_table.h"

#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MIN
#  define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#  define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

static LilvNode* atom_AtomPort   = NULL;
static LilvNode* atom_Sequence   = NULL;
static LilvNode* lv2_AudioPort   = NULL;
static LilvNode* lv2_CVPort      = NULL;
static LilvNode* lv2_ControlPort = NULL;
static LilvNode* lv2_InputPort   = NULL;
static LilvNode* lv2_OutputPort  = NULL;
static LilvNode* urid_map        = NULL;

static void
print_version(void)
{
  printf("lv2bench (lilv) " LILV_VERSION "\n");
}

static int
print_usage(const char* const name, const int status)
{
  FILE* const out = status ? stderr : stdout;
  fprintf(out, "Usage: %s [OPTION]... [PLUGIN_URI]\n", name);
  fprintf(out,
          "Benchmark installed LV2 plugins.\n\n"
          "  -V, --version        Print version information and exit\n"
          "  -b, --block FRAMES   Block length to run, in audio frames\n"
          "  -h, --help           Print this help and exit\n"
          "  -n, --length FRAMES  Total number of frames to process\n");
  return status;
}

static int
bench(const LilvPlugin* const p,
      const uint32_t          sample_count,
      const uint32_t          block_size)
{
  static const size_t atom_capacity = 1024;

  URITable uri_table;
  uri_table_init(&uri_table);

  LV2_URID_Map       map           = {&uri_table, uri_table_map};
  LV2_URID_Unmap     unmap         = {&uri_table, uri_table_unmap};
  const LV2_Feature  map_feature   = {LV2_URID_MAP_URI, &map};
  const LV2_Feature  unmap_feature = {LV2_URID_UNMAP_URI, &unmap};
  const LV2_Feature* features[]    = {&map_feature, &unmap_feature, NULL};

  float* const buf = (float*)calloc(block_size * 2UL, sizeof(float));
  if (!buf) {
    fprintf(stderr, "error: Out of memory\n");
    return 1;
  }

  float* const in  = buf;
  float* const out = buf + block_size;

  LV2_Atom_Sequence seq_in = {{sizeof(LV2_Atom_Sequence_Body),
                               uri_table_map(&uri_table, LV2_ATOM__Sequence)},
                              {0, 0}};

  LV2_Atom_Sequence* seq_out =
    (LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + atom_capacity);

  const char* const uri      = lilv_node_as_string(lilv_plugin_get_uri(p));
  LilvNodes* const  required = lilv_plugin_get_required_features(p);
  LILV_FOREACH (nodes, i, required) {
    const LilvNode* const feature = lilv_nodes_get(required, i);
    if (!lilv_node_equals(feature, urid_map)) {
      fprintf(stderr,
              "warning: <%s> requires feature <%s>, skipping\n",
              uri,
              lilv_node_as_uri(feature));
      free(seq_out);
      free(buf);
      uri_table_destroy(&uri_table);
      return 2;
    }
  }
  lilv_nodes_free(required);

  LilvInstance* const instance = lilv_plugin_instantiate(p, 48000.0, features);
  if (!instance) {
    fprintf(stderr,
            "warning: Failed to instantiate <%s>\n",
            lilv_node_as_uri(lilv_plugin_get_uri(p)));
    free(seq_out);
    free(buf);
    uri_table_destroy(&uri_table);
    return 3;
  }

  const uint32_t n_ports  = lilv_plugin_get_num_ports(p);
  float* const   mins     = (float*)calloc(n_ports, sizeof(float));
  float* const   maxes    = (float*)calloc(n_ports, sizeof(float));
  float* const   controls = (float*)calloc(n_ports, sizeof(float));
  lilv_plugin_get_port_ranges_float(p, mins, maxes, controls);

  bool skip_plugin = false;
  for (uint32_t index = 0; !skip_plugin && index < n_ports; ++index) {
    const LilvPort* const port = lilv_plugin_get_port_by_index(p, index);
    if (lilv_port_is_a(p, port, lv2_ControlPort)) {
      if (isnan(controls[index])) {
        if (!isnan(mins[index])) {
          controls[index] = mins[index];
        } else if (!isnan(maxes[index])) {
          controls[index] = maxes[index];
        } else {
          controls[index] = 0.0;
        }
      }
      lilv_instance_connect_port(instance, index, &controls[index]);
    } else if (lilv_port_is_a(p, port, lv2_AudioPort) ||
               lilv_port_is_a(p, port, lv2_CVPort)) {
      if (lilv_port_is_a(p, port, lv2_InputPort)) {
        lilv_instance_connect_port(instance, index, in);
      } else if (lilv_port_is_a(p, port, lv2_OutputPort)) {
        lilv_instance_connect_port(instance, index, out);
      } else {
        fprintf(stderr,
                "warning: <%s> port %u neither input nor output, skipping\n",
                uri,
                index);
        skip_plugin = true;
      }
    } else if (lilv_port_is_a(p, port, atom_AtomPort)) {
      if (lilv_port_is_a(p, port, lv2_InputPort)) {
        lilv_instance_connect_port(instance, index, &seq_in);
      } else {
        lilv_instance_connect_port(instance, index, seq_out);
      }
    } else {
      fprintf(stderr, "<%s> port %u has unknown type, skipping\n", uri, index);
      skip_plugin = true;
    }
  }

  if (!skip_plugin) {
    lilv_instance_activate(instance);

    const BenchmarkTime benchmark_start = bench_start();
    const uint32_t      n_blocks        = sample_count / block_size;
    double              buffer_min      = DBL_MAX;
    double              buffer_max      = 0.0;

    for (uint32_t i = 0; i < n_blocks; ++i) {
      seq_in.atom.size   = sizeof(LV2_Atom_Sequence_Body);
      seq_in.atom.type   = uri_table_map(&uri_table, LV2_ATOM__Sequence);
      seq_out->atom.size = atom_capacity;
      seq_out->atom.type = uri_table_map(&uri_table, LV2_ATOM__Chunk);

      const BenchmarkTime buffer_start = bench_start();

      lilv_instance_run(instance, block_size);

      const double buffer_elapsed = bench_end(&buffer_start);

      buffer_min = MIN(buffer_min, buffer_elapsed);
      buffer_max = MAX(buffer_max, buffer_elapsed);
    }

    const double benchmark_elapsed = bench_end(&benchmark_start);
    const double buffer_mean       = benchmark_elapsed / n_blocks;

    printf("%u\t%u\t%.9g\t%.9g\t%.9g\t%.9g\t%s\n",
           block_size,
           sample_count,
           buffer_min,
           buffer_mean,
           buffer_max,
           benchmark_elapsed,
           uri);

    lilv_instance_deactivate(instance);
  }

  free(controls);
  free(maxes);
  free(mins);
  lilv_instance_free(instance);
  free(seq_out);
  free(buf);
  uri_table_destroy(&uri_table);
  return 0;
}

int
main(const int argc, char** const argv)
{
  uint32_t block_size   = 512;
  uint32_t sample_count = (1U << 19U);

  int a = 1;
  for (; a < argc; ++a) {
    if (!strcmp(argv[a], "-V") || !strcmp(argv[a], "--version")) {
      print_version();
      return 0;
    }

    if (!strcmp(argv[a], "-h") || !strcmp(argv[a], "--help")) {
      return print_usage(argv[0], 0);
    }

    if ((!strcmp(argv[a], "-n") || !strcmp(argv[a], "--frames")) &&
        (a + 1 < argc)) {
      const long l = strtol(argv[++a], NULL, 10);
      if (l > 0 && (unsigned long)l < (1UL << 28UL)) {
        sample_count = (uint32_t)l;
      }
    } else if (!strcmp(argv[a], "-b") && (a + 1 < argc)) {
      const long l = strtol(argv[++a], NULL, 10);
      if (l > 0 && l < 16384) {
        block_size = (uint32_t)l;
      }
    } else if (argv[a][0] == '-') {
      return print_usage(argv[0], 1);
    }
  }

  const char* const plugin_uri_str = (a < argc ? argv[a++] : NULL);

  LilvWorld* const world = lilv_world_new();
  lilv_world_set_option(world, LILV_OPTION_OBJECT_INDEX, NULL);
  lilv_world_load_all(world);

  atom_AtomPort   = lilv_new_uri(world, LV2_ATOM__AtomPort);
  atom_Sequence   = lilv_new_uri(world, LV2_ATOM__Sequence);
  lv2_AudioPort   = lilv_new_uri(world, LV2_CORE__AudioPort);
  lv2_CVPort      = lilv_new_uri(world, LV2_CORE__CVPort);
  lv2_ControlPort = lilv_new_uri(world, LV2_CORE__ControlPort);
  lv2_InputPort   = lilv_new_uri(world, LV2_CORE__InputPort);
  lv2_OutputPort  = lilv_new_uri(world, LV2_CORE__OutputPort);
  urid_map        = lilv_new_uri(world, LV2_URID__map);

  printf("Block\tFrames\tMin\tMean\tMax\tTotal\tPlugin\n");

  const LilvPlugins* const plugins     = lilv_world_get_all_plugins(world);
  int                      exit_status = 0;
  if (plugin_uri_str) {
    LilvNode* const uri = lilv_new_uri(world, plugin_uri_str);

    exit_status =
      bench(lilv_plugins_get_by_uri(plugins, uri), sample_count, block_size);

    lilv_node_free(uri);
  } else {
    LILV_FOREACH (plugins, i, plugins) {
      const int st =
        bench(lilv_plugins_get(plugins, i), sample_count, block_size);

      exit_status = exit_status ? exit_status : st;
    }
  }

  lilv_node_free(urid_map);
  lilv_node_free(lv2_OutputPort);
  lilv_node_free(lv2_InputPort);
  lilv_node_free(lv2_ControlPort);
  lilv_node_free(lv2_CVPort);
  lilv_node_free(lv2_AudioPort);
  lilv_node_free(atom_Sequence);
  lilv_node_free(atom_AtomPort);

  lilv_world_free(world);

  return exit_status;
}
