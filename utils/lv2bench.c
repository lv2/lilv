/*
  Copyright 2012-2019 David Robillard <d@drobilla.net>

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

#define _POSIX_C_SOURCE 200809L

#include "lilv/lilv.h"
#include "lv2/atom/atom.h"
#include "lv2/core/lv2.h"
#include "lv2/urid/urid.h"

#include "bench.h"
#include "lilv_config.h"
#include "uri_table.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <float.h>

#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

static LilvNode* atom_AtomPort   = NULL;
static LilvNode* atom_Sequence   = NULL;
static LilvNode* lv2_AudioPort   = NULL;
static LilvNode* lv2_CVPort      = NULL;
static LilvNode* lv2_ControlPort = NULL;
static LilvNode* lv2_InputPort   = NULL;
static LilvNode* lv2_OutputPort  = NULL;
static LilvNode* urid_map        = NULL;

static bool full_output          = false;
static bool output_all           = false;
static bool skip_first           = false;
static bool report_microseconds  = false;
static bool report_milliseconds  = false;
static int  realtime_priority    = -1;
static bool lock_memory          = false;
static bool aggregate_statistics = false;


static void
print_version(void)
{
  printf("lv2bench (lilv) " LILV_VERSION "\n"
         "Copyright 2012-2021 David Robillard <d@drobilla.net>\n"
         "License: <http://www.opensource.org/licenses/isc-license>\n"
         "This is free software: you are free to change and redistribute it.\n"
         "There is NO WARRANTY, to the extent permitted by law.\n");
}

static void
print_usage(void)
{
  printf("lv2bench - Benchmark all installed and supported LV2 plugins.\n");
  printf("Usage: lv2bench [OPTIONS] [PLUGIN_URI]\n");
  printf("\n");
  printf("  -h, --help        Display this help and exit.\n");
  printf("  -b BLOCK_SIZE     Specify block size, in audio frames.\n");
  printf("  -n FRAMES         Total number of audio frames to process\n");
  printf("  -u, --usecs       Report microseconds instead of seconds\n");
  printf("  -m, --msecs       Report milliseconds instead of seconds\n");
  printf("  -f, --full        Full plottable output.\n");
  printf("  -a, --all-buffers Output elapsed times for all individual buffers (for benchmarking a single plugin only)\n");
  printf("  -s, --skip-first  Skip measuring the first buffer processing time\n");
  printf("  -l, --lock        Lock memory into RAM (mlockall)\n");
  printf("  -p PRIORITY       Process priority (also sets scheduling class SCHED_FIFO)\n");
  printf("  --version         Display version information and exit\n");
}

static double
bench(const LilvPlugin* p, uint32_t sample_count, uint32_t block_size)
{
  URITable uri_table;
  uri_table_init(&uri_table);

  LV2_URID_Map       map           = {&uri_table, uri_table_map};
  LV2_Feature        map_feature   = {LV2_URID_MAP_URI, &map};
  LV2_URID_Unmap     unmap         = {&uri_table, uri_table_unmap};
  LV2_Feature        unmap_feature = {LV2_URID_UNMAP_URI, &unmap};
  const LV2_Feature* features[]    = {&map_feature, &unmap_feature, NULL};

  float* const buf = (float*)calloc(block_size * 2ul, sizeof(float));
  float* const in  = buf;
  float* const out = buf + block_size;
  if (!buf) {
    fprintf(stderr, "Out of memory\n");
    return 0.0;
  }

  const size_t atom_capacity = 1024;

  LV2_Atom_Sequence seq_in = {{sizeof(LV2_Atom_Sequence_Body),
                               uri_table_map(&uri_table, LV2_ATOM__Sequence)},
                              {0, 0}};

  LV2_Atom_Sequence* seq_out =
    (LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + atom_capacity);

  const char* uri      = lilv_node_as_string(lilv_plugin_get_uri(p));
  LilvNodes*  required = lilv_plugin_get_required_features(p);
  LILV_FOREACH (nodes, i, required) {
    const LilvNode* feature = lilv_nodes_get(required, i);
    if (!lilv_node_equals(feature, urid_map)) {
      fprintf(stderr,
              "<%s> requires feature <%s>, skipping\n",
              uri,
              lilv_node_as_uri(feature));
      free(seq_out);
      free(buf);
      uri_table_destroy(&uri_table);
      return 0.0;
    }
  }

  LilvInstance* instance = lilv_plugin_instantiate(p, 48000.0, features);
  if (!instance) {
    fprintf(stderr,
            "Failed to instantiate <%s>\n",
            lilv_node_as_uri(lilv_plugin_get_uri(p)));
    free(seq_out);
    free(buf);
    uri_table_destroy(&uri_table);
    return 0.0;
  }

  const uint32_t n_ports  = lilv_plugin_get_num_ports(p);
  float* const   mins     = (float*)calloc(n_ports, sizeof(float));
  float* const   maxes    = (float*)calloc(n_ports, sizeof(float));
  float* const   controls = (float*)calloc(n_ports, sizeof(float));
  lilv_plugin_get_port_ranges_float(p, mins, maxes, controls);

  for (uint32_t index = 0; index < n_ports; ++index) {
    const LilvPort* port = lilv_plugin_get_port_by_index(p, index);
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
                "<%s> port %u neither input nor output, skipping\n",
                uri,
                index);
        lilv_instance_free(instance);
        free(seq_out);
        free(buf);
        free(controls);
        uri_table_destroy(&uri_table);
        return 0.0;
      }
    } else if (lilv_port_is_a(p, port, atom_AtomPort)) {
      if (lilv_port_is_a(p, port, lv2_InputPort)) {
        lilv_instance_connect_port(instance, index, &seq_in);
      } else {
        lilv_instance_connect_port(instance, index, seq_out);
      }
    } else {
      fprintf(stderr, "<%s> port %u has unknown type, skipping\n", uri, index);
      lilv_instance_free(instance);
      free(seq_out);
      free(buf);
      free(controls);
      uri_table_destroy(&uri_table);
      return 0.0;
    }
  }

  if (lock_memory) {
    if (mlockall(MCL_CURRENT) != 0) {
      fprintf(stderr, "Failed to lock memory: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  lilv_instance_activate(instance);

  if (realtime_priority != -1) {
    struct sched_param sp;
    sp.sched_priority = realtime_priority;
    if (sched_setscheduler(getpid(), SCHED_FIFO, &sp) != 0) {
      fprintf(stderr, "Failed to set realtime scheduling SCHED_FIFO at priority %d: %s\n", realtime_priority, strerror(errno));
      exit(EXIT_FAILURE); 
    }
  }

  double time_base = 1.0;
  if (report_milliseconds) {
    time_base = 1000.0;
  }
  if (report_microseconds) {
    time_base = 1000000.0;
  }

  double *elapsed = 0;
  if (output_all) {
    elapsed = malloc(sizeof(double) * (sample_count / block_size));
  }

  double elapsed_min = FLT_MAX;
  double elapsed_max = 0.f;
  double elapsed_total = 0.f;

  for (uint32_t i = 0; i < (sample_count / block_size); ++i) {
    seq_in.atom.size   = sizeof(LV2_Atom_Sequence_Body);
    seq_in.atom.type   = uri_table_map(&uri_table, LV2_ATOM__Sequence);
    seq_out->atom.size = atom_capacity;
    seq_out->atom.type = uri_table_map(&uri_table, LV2_ATOM__Chunk);

    BenchmarkTime ts_buffer = bench_start();
    lilv_instance_run(instance, block_size);
    const double elapsed_buffer = bench_end(&ts_buffer);

    if (skip_first && i == 0) {
      continue;
    }

    elapsed_total += elapsed_buffer;
    if (output_all) {
      elapsed[i] = elapsed_buffer;
    }

    if (elapsed_buffer < elapsed_min) elapsed_min = elapsed_buffer;
    if (elapsed_buffer > elapsed_max) elapsed_max = elapsed_buffer; 
  }

  if (realtime_priority != -1) {
    struct sched_param sp;
    sp.sched_priority = 0;
    if (sched_setscheduler(getpid(), SCHED_OTHER, &sp) != 0) {
      fprintf(stderr, "Failed to set realtime scheduling SCHED_FIFO at priority %d: %s\n", realtime_priority, strerror(errno));
      exit(EXIT_FAILURE); 
    }
  }

  lilv_instance_deactivate(instance);
  lilv_instance_free(instance);
  free(controls);
  free(maxes);
  free(mins);
  free(seq_out);

  uri_table_destroy(&uri_table);

  if (full_output) {
    printf("%u %u %lf %lf %lf ", block_size, sample_count, time_base * elapsed_min, time_base * elapsed_total / (sample_count / block_size), time_base * elapsed_max);
  }
  printf("%lf %s\n", time_base * elapsed_total, uri);

  if (output_all) {
    printf("# Buffer Elapsed\n");
    for (uint32_t i = 0; i < (sample_count / block_size); ++i) {
      if (skip_first && i == 0) {
        continue;
      }
      printf("%d %f\n", i, time_base * elapsed[i]);
    }
    free(elapsed);
  }

  free(buf);
  return elapsed_total;
}

int
main(int argc, char** argv)
{
  uint32_t block_size   = 512;
  uint32_t sample_count = (1 << 19);

  int a = 1;
  for (; a < argc; ++a) {
    if (!strcmp(argv[a], "--version")) {
      print_version();
      return 0;
    }

    if (!strcmp(argv[a], "--help")) {
      print_usage();
      return 0;
    }

    if (!strcmp(argv[a], "-f")) {
      full_output = true;
    } else if (!strcmp(argv[a], "-l")) {
      lock_memory = true;
    } else if (!strcmp(argv[a], "-a")) {
      output_all = true;
    } else if (!strcmp(argv[a], "-s")) {
      skip_first = true;
    } else if (!strcmp(argv[a], "-m")) {
      report_milliseconds = true;
    } else if (!strcmp(argv[a], "-u")) {
      report_microseconds = true;
    } else if (!strcmp(argv[a], "-p") && (a + 1 < argc)) {
      realtime_priority = atoi(argv[++a]);
    } else if (!strcmp(argv[a], "-n") && (a + 1 < argc)) {
      sample_count = atoi(argv[++a]);
    } else if (!strcmp(argv[a], "-b") && (a + 1 < argc)) {
      block_size = atoi(argv[++a]);
    } else if (argv[a][0] != '-') {
      break;
    } else {
      print_usage();
      return 1;
    }
  }

  const char* const plugin_uri_str = (a < argc ? argv[a++] : NULL);

  LilvWorld* world = lilv_world_new();
  lilv_world_load_all(world);

  atom_AtomPort   = lilv_new_uri(world, LV2_ATOM__AtomPort);
  atom_Sequence   = lilv_new_uri(world, LV2_ATOM__Sequence);
  lv2_AudioPort   = lilv_new_uri(world, LV2_CORE__AudioPort);
  lv2_CVPort      = lilv_new_uri(world, LV2_CORE__CVPort);
  lv2_ControlPort = lilv_new_uri(world, LV2_CORE__ControlPort);
  lv2_InputPort   = lilv_new_uri(world, LV2_CORE__InputPort);
  lv2_OutputPort  = lilv_new_uri(world, LV2_CORE__OutputPort);
  urid_map        = lilv_new_uri(world, LV2_URID__map);

  if (full_output) {
    printf("# Blocksize Frames Minimum Average Maximum Total Plugin(URI)\n");
  }

  const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
  if (plugin_uri_str) {
    LilvNode* uri = lilv_new_uri(world, plugin_uri_str);
    bench(lilv_plugins_get_by_uri(plugins, uri), sample_count, block_size);
    lilv_node_free(uri);
  } else {
    LILV_FOREACH (plugins, i, plugins) {
      bench(lilv_plugins_get(plugins, i), sample_count, block_size);
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

  return 0;
}
