// Copyright 2012-2024 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "bench.h"
#include "uri_table.h"

#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>

#include <errno.h>
#include <float.h>
#include <math.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#ifndef MIN
#  define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#  define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

typedef struct {
  uint32_t block_size;
  uint32_t sample_count;
  uint32_t skip_buffers;
  int      realtime_priority;
  bool     lock_memory;
  bool     output_all;
} Options;

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
  printf("lv2bench (lilv) " LILV_VERSION "\n"
         "Copyright 2012-2024 David Robillard <d@drobilla.net>\n"
         "License: <http://www.opensource.org/licenses/isc-license>\n"
         "This is free software: you are free to change and redistribute it.\n"
         "There is NO WARRANTY, to the extent permitted by law.\n");
}

static void
print_usage(void)
{
  printf("Usage: lv2bench [OPTION]... [PLUGIN_URI]\n"
         "Benchmark LV2 plugins.\n"
         "\n"
         "  -V, --version  Display version information and exit\n"
         "  -a             Output timings for all individual buffers\n"
         "  -b BLOCK_SIZE  Block size in audio frames\n"
         "  -h, --help     Display this help and exit\n"
         "  -l             Lock memory into RAM\n"
         "  -n FRAMES      Total number of frames to process\n"
         "  -p PRIORITY    Real-time process priority\n"
         "  -s BUFFERS     Skip initial buffers before recording timings\n");
}

static int
bench(const LilvPlugin* const p, const Options options)
{
  static const size_t atom_capacity = 2097152;

  URITable uri_table;
  uri_table_init(&uri_table);

  LV2_URID_Map       map           = {&uri_table, uri_table_map};
  LV2_URID_Unmap     unmap         = {&uri_table, uri_table_unmap};
  const LV2_Feature  map_feature   = {LV2_URID_MAP_URI, &map};
  const LV2_Feature  unmap_feature = {LV2_URID_UNMAP_URI, &unmap};
  const LV2_Feature* features[]    = {&map_feature, &unmap_feature, NULL};

  float* const buf = (float*)calloc(options.block_size * 2UL, sizeof(float));
  if (!buf) {
    fprintf(stderr, "error: Out of memory\n");
    return 1;
  }

  float* const in  = buf;
  float* const out = buf + options.block_size;

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

  for (uint32_t index = 0; index < n_ports; ++index) {
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
        lilv_instance_free(instance);
        free(seq_out);
        free(buf);
        free(controls);
        uri_table_destroy(&uri_table);
        return 4;
      }
    } else if (lilv_port_is_a(p, port, atom_AtomPort)) {
      if (lilv_port_is_a(p, port, lv2_InputPort)) {
        lilv_instance_connect_port(instance, index, &seq_in);
      } else {
        lilv_instance_connect_port(instance, index, seq_out);
      }
    } else {
      fprintf(stderr,
              "warning: <%s> port %u has unknown type, skipping\n",
              uri,
              index);
      lilv_instance_free(instance);
      free(seq_out);
      free(buf);
      free(controls);
      uri_table_destroy(&uri_table);
      return 5;
    }
  }

  if (options.lock_memory) {
    if (mlockall(MCL_CURRENT) != 0) {
      fprintf(stderr, "Failed to lock memory: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  lilv_instance_activate(instance);

  if (options.realtime_priority != -1) {
    struct sched_param sp;
    sp.sched_priority = options.realtime_priority;
    if (sched_setscheduler(getpid(), SCHED_FIFO, &sp) != 0) {
      fprintf(
        stderr,
        "Failed to set realtime scheduling SCHED_FIFO at priority %d: %s\n",
        options.realtime_priority,
        strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  const uint32_t n_blocks = options.sample_count / options.block_size;
  seq_in.atom.size        = sizeof(LV2_Atom_Sequence_Body);
  seq_in.atom.type        = uri_table_map(&uri_table, LV2_ATOM__Sequence);

  // Pre-roll plugin to skip initial buffers if requested
  for (uint32_t i = 0; i < options.skip_buffers; ++i) {
    seq_out->atom.size = atom_capacity;
    seq_out->atom.type = uri_table_map(&uri_table, LV2_ATOM__Chunk);
    lilv_instance_run(instance, options.block_size);
  }

  double* elapsed = 0;
  if (options.output_all) {
    elapsed = (double*)calloc(n_blocks, sizeof(double));
  }

  const BenchmarkTime benchmark_start = bench_start();
  double              buffer_min      = DBL_MAX;
  double              buffer_max      = 0.0;

  // Run plugin and record buffer timings
  for (uint32_t i = 0; i < n_blocks; ++i) {
    seq_out->atom.size = atom_capacity;
    seq_out->atom.type = uri_table_map(&uri_table, LV2_ATOM__Chunk);

    const BenchmarkTime buffer_start = bench_start();

    lilv_instance_run(instance, options.block_size);

    const double buffer_elapsed = bench_end(&buffer_start);

    // Give the system a chance to do its bookkeeping
    if (options.realtime_priority != -1) {
      const struct timespec pause = {0, 1000000};
      nanosleep(&pause, NULL);
    }

    if (options.output_all) {
      elapsed[i] = buffer_elapsed;
    }

    buffer_min = MIN(buffer_min, buffer_elapsed);
    buffer_max = MAX(buffer_max, buffer_elapsed);
  }

  if (options.realtime_priority != -1) {
    struct sched_param sp;
    sp.sched_priority = 0;
    if (sched_setscheduler(getpid(), SCHED_OTHER, &sp) != 0) {
      fprintf(
        stderr,
        "Failed to set realtime scheduling SCHED_FIFO at priority %d: %s\n",
        options.realtime_priority,
        strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  const double benchmark_elapsed = bench_end(&benchmark_start);
  const double buffer_mean       = benchmark_elapsed / n_blocks;

  lilv_instance_deactivate(instance);
  lilv_instance_free(instance);
  free(controls);
  free(maxes);
  free(mins);
  free(seq_out);

  uri_table_destroy(&uri_table);

  printf("%u\t%u\t%.9g\t%.9g\t%.9g\t%.9g\t%s\n",
         options.block_size,
         options.sample_count,
         buffer_min,
         buffer_mean,
         buffer_max,
         benchmark_elapsed,
         uri);

  if (options.output_all) {
    printf("# Buffer\tElapsed\n");
    for (uint32_t i = 0; i < n_blocks; ++i) {
      printf("%u\t%.9g\n", i, elapsed[i]);
    }
    free(elapsed);
  }

  free(buf);
  return 0;
}

int
main(const int argc, char** const argv)
{
  Options options = {512U, (1U << 19U), 0U, -1, false, false};

  int a = 1;
  for (; a < argc; ++a) {
    if (!strcmp(argv[a], "-V") || !strcmp(argv[a], "--version")) {
      print_version();
      return 0;
    }

    if (!strcmp(argv[a], "-h") || !strcmp(argv[a], "--help")) {
      print_usage();
      return 0;
    }

    if (!strcmp(argv[a], "-a")) {
      options.output_all = true;
    } else if (!strcmp(argv[a], "-b") && (a + 1 < argc)) {
      options.block_size = atoi(argv[++a]);
    } else if (!strcmp(argv[a], "-l")) {
      options.lock_memory = true;
    } else if (!strcmp(argv[a], "-n") && (a + 1 < argc)) {
      options.sample_count = atoi(argv[++a]);
    } else if (!strcmp(argv[a], "-p") && (a + 1 < argc)) {
      options.realtime_priority = atoi(argv[++a]);
    } else if (!strcmp(argv[a], "-s") && (a + 1 < argc)) {
      options.skip_buffers = atoi(argv[++a]);
    } else if (argv[a][0] != '-') {
      break;
    } else {
      print_usage();
      return 1;
    }
  }

  const char* const plugin_uri_str = (a < argc ? argv[a++] : NULL);

  LilvWorld* const world = lilv_world_new();
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

    exit_status = bench(lilv_plugins_get_by_uri(plugins, uri), options);

    lilv_node_free(uri);
  } else {
    LILV_FOREACH (plugins, i, plugins) {
      const int st = bench(lilv_plugins_get(plugins, i), options);

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
