// Copyright 2011-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <lv2/atom/atom.h>
#include <lv2/core/lv2.h>
#include <lv2/state/state.h>
#include <lv2/urid/urid.h>
#include <zix/filesystem.h>
#include <zix/path.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_URI "http://example.org/lilv-test-plugin"

enum { TEST_INPUT = 0, TEST_OUTPUT = 1, TEST_CONTROL = 2 };

typedef struct {
  LV2_URID_Map*        map;
  LV2_State_Free_Path* free_path;

  struct {
    LV2_URID atom_Float;
  } uris;

  char* tmp_dir_path;
  char* rec_file_path;
  FILE* rec_file;

  float*   input;
  float*   output;
  unsigned num_runs;
} Test;

static void
cleanup(LV2_Handle instance)
{
  Test* test = (Test*)instance;
  if (test->rec_file) {
    fclose(test->rec_file);
  }

  if (test->free_path) {
    test->free_path->free_path(test->free_path->handle, test->rec_file_path);
  }

  free(test->tmp_dir_path);
  free(instance);
}

static void
connect_port(LV2_Handle instance, uint32_t port, void* data)
{
  Test* test = (Test*)instance;
  switch (port) {
  case TEST_INPUT:
    test->input = (float*)data;
    break;
  case TEST_OUTPUT:
    test->output = (float*)data;
    break;
  case TEST_CONTROL:
    test->output = (float*)data;
    break;
  default:
    break;
  }
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               path,
            const LV2_Feature* const* features)
{
  (void)descriptor;
  (void)rate;
  (void)path;

  Test* test = (Test*)calloc(1, sizeof(Test));
  if (!test) {
    return NULL;
  }

  test->tmp_dir_path = zix_temp_directory_path(NULL);

  LV2_State_Make_Path* make_path = NULL;

  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
      test->map = (LV2_URID_Map*)features[i]->data;
      test->uris.atom_Float =
        test->map->map(test->map->handle, LV2_ATOM__Float);
    } else if (!strcmp(features[i]->URI, LV2_STATE__makePath)) {
      make_path = (LV2_State_Make_Path*)features[i]->data;
    } else if (!strcmp(features[i]->URI, LV2_STATE__freePath)) {
      test->free_path = (LV2_State_Free_Path*)features[i]->data;
    }
  }

  if (!test->map) {
    fprintf(stderr, "Host does not support urid:map\n");
    free(test);
    return NULL;
  }

  if (make_path) {
    if (!test->free_path) {
      fprintf(stderr, "Host provided make_path without free_path\n");
      free(test);
      return NULL;
    }

    test->rec_file_path = make_path->path(make_path->handle, "recfile");
    if (!(test->rec_file = fopen(test->rec_file_path, "w"))) {
      fprintf(stderr, "ERROR: Failed to open rec file\n");
    } else {
      fprintf(test->rec_file, "instantiate\n");
    }
  }

  return (LV2_Handle)test;
}

static void
run(LV2_Handle instance, uint32_t sample_count)
{
  Test* test    = (Test*)instance;
  *test->output = *test->input;
  if (sample_count == 1) {
    ++test->num_runs;
  } else if (sample_count == 2 && test->rec_file) {
    // Append to rec file (changes size)
    fprintf(test->rec_file, "run\n");
  } else if (sample_count == 3 && test->rec_file) {
    // Change the first byte of rec file (doesn't change size)
    fseek(test->rec_file, 0, SEEK_SET);
    fprintf(test->rec_file, "X");
    fseek(test->rec_file, 0, SEEK_END);
  }
}

static uint32_t
map_uri(Test* plugin, const char* uri)
{
  return plugin->map->map(plugin->map->handle, uri);
}

static LV2_State_Status
save(LV2_Handle                instance,
     LV2_State_Store_Function  store,
     void*                     callback_data,
     uint32_t                  flags,
     const LV2_Feature* const* features)
{
  (void)flags;

  Test* plugin = (Test*)instance;

  LV2_State_Map_Path*  map_path  = NULL;
  LV2_State_Make_Path* make_path = NULL;
  LV2_State_Free_Path* free_path = NULL;
  for (int i = 0; features && features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_STATE__mapPath)) {
      map_path = (LV2_State_Map_Path*)features[i]->data;
    } else if (!strcmp(features[i]->URI, LV2_STATE__makePath)) {
      make_path = (LV2_State_Make_Path*)features[i]->data;
    } else if (!strcmp(features[i]->URI, LV2_STATE__freePath)) {
      free_path = (LV2_State_Free_Path*)features[i]->data;
    }
  }

  if (!map_path || !free_path) {
    return LV2_STATE_ERR_NO_FEATURE;
  }

  store(callback_data,
        map_uri(plugin, "http://example.org/greeting"),
        "hello",
        strlen("hello") + 1,
        map_uri(plugin, LV2_ATOM__String),
        LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  const uint32_t urid = map_uri(plugin, "http://example.org/urivalue");
  store(callback_data,
        map_uri(plugin, "http://example.org/uri"),
        &urid,
        sizeof(uint32_t),
        map_uri(plugin, LV2_ATOM__URID),
        LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  // Try to store second value for the same property (should fail)
  const uint32_t urid2 = map_uri(plugin, "http://example.org/urivalue2");
  if (!store(callback_data,
             map_uri(plugin, "http://example.org/uri"),
             &urid2,
             sizeof(uint32_t),
             map_uri(plugin, LV2_ATOM__URID),
             LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE)) {
    return LV2_STATE_ERR_UNKNOWN;
  }

  // Try to store with a null key (should fail)
  if (!store(callback_data,
             0,
             &urid2,
             sizeof(uint32_t),
             map_uri(plugin, LV2_ATOM__URID),
             LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE)) {
    return LV2_STATE_ERR_UNKNOWN;
  }

  store(callback_data,
        map_uri(plugin, "http://example.org/num-runs"),
        &plugin->num_runs,
        sizeof(plugin->num_runs),
        map_uri(plugin, LV2_ATOM__Int),
        LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  const float two = 2.0f;
  store(callback_data,
        map_uri(plugin, "http://example.org/two"),
        &two,
        sizeof(two),
        map_uri(plugin, LV2_ATOM__Float),
        LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  const uint32_t affirmative = 1;
  store(callback_data,
        map_uri(plugin, "http://example.org/true"),
        &affirmative,
        sizeof(affirmative),
        map_uri(plugin, LV2_ATOM__Bool),
        LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  const uint32_t negative = 0;
  store(callback_data,
        map_uri(plugin, "http://example.org/false"),
        &negative,
        sizeof(negative),
        map_uri(plugin, LV2_ATOM__Bool),
        LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  const uint8_t blob[] = "I am a blob of arbitrary data.";
  store(callback_data,
        map_uri(plugin, "http://example.org/blob"),
        blob,
        sizeof(blob),
        map_uri(plugin, "http://example.org/SomeUnknownType"),
        LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  if (map_path) {
    const char* const file_name = "temp_file.txt";

    char* const tmp_file_path =
      zix_path_join(NULL, plugin->tmp_dir_path, file_name);

    FILE* file = fopen(tmp_file_path, "w");
    if (!file) {
      fprintf(stderr, "error: Failed to open file %s\n", tmp_file_path);
      free(tmp_file_path);
      return LV2_STATE_ERR_UNKNOWN;
    }

    fprintf(file, "Hello\n");
    fclose(file);

    char* apath  = map_path->abstract_path(map_path->handle, tmp_file_path);
    char* apath2 = map_path->abstract_path(map_path->handle, tmp_file_path);
    free(tmp_file_path);
    if (!!strcmp(apath, apath2)) {
      fprintf(stderr, "error: Path %s != %s\n", apath, apath2);
    }

    store(callback_data,
          map_uri(plugin, "http://example.org/extfile"),
          apath,
          strlen(apath) + 1,
          map_uri(plugin, LV2_ATOM__Path),
          LV2_STATE_IS_POD);

    free_path->free_path(free_path->handle, apath);
    free_path->free_path(free_path->handle, apath2);

    if (plugin->rec_file) {
      fflush(plugin->rec_file);
      apath = map_path->abstract_path(map_path->handle, plugin->rec_file_path);

      store(callback_data,
            map_uri(plugin, "http://example.org/recfile"),
            apath,
            strlen(apath) + 1,
            map_uri(plugin, LV2_ATOM__Path),
            LV2_STATE_IS_POD);

      free_path->free_path(free_path->handle, apath);
    }

    if (make_path) {
      char* spath = make_path->path(make_path->handle, "save");
      FILE* sfile = fopen(spath, "w");
      if (sfile) {
        fprintf(sfile, "save");
        fclose(sfile);
      }

      apath = map_path->abstract_path(map_path->handle, spath);
      store(callback_data,
            map_uri(plugin, "http://example.org/save-file"),
            apath,
            strlen(apath) + 1,
            map_uri(plugin, LV2_ATOM__Path),
            LV2_STATE_IS_POD);
      free_path->free_path(free_path->handle, apath);
      free_path->free_path(free_path->handle, spath);
    }
  }

  return LV2_STATE_SUCCESS;
}

static LV2_State_Status
restore(LV2_Handle                  instance,
        LV2_State_Retrieve_Function retrieve,
        void*                       callback_data,
        uint32_t                    flags,
        const LV2_Feature* const*   features)
{
  (void)flags;

  Test* plugin = (Test*)instance;

  LV2_State_Map_Path*  map_path  = NULL;
  LV2_State_Free_Path* free_path = NULL;
  for (int i = 0; features && features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_STATE__mapPath)) {
      map_path = (LV2_State_Map_Path*)features[i]->data;
    } else if (!strcmp(features[i]->URI, LV2_STATE__freePath)) {
      free_path = (LV2_State_Free_Path*)features[i]->data;
    }
  }

  size_t   size     = 0;
  uint32_t type     = 0;
  uint32_t valflags = 0;

  plugin->num_runs =
    *(int32_t*)retrieve(callback_data,
                        map_uri(plugin, "http://example.org/num-runs"),
                        &size,
                        &type,
                        &valflags);

  if (!map_path || !free_path) {
    return LV2_STATE_ERR_NO_FEATURE;
  }

  char* apath = (char*)retrieve(callback_data,
                                map_uri(plugin, "http://example.org/extfile"),
                                &size,
                                &type,
                                &valflags);

  if (valflags != LV2_STATE_IS_POD) {
    fprintf(stderr, "error: Restored bad file flags\n");
    return LV2_STATE_ERR_BAD_FLAGS;
  }

  if (apath) {
    char* path = map_path->absolute_path(map_path->handle, apath);
    FILE* f    = fopen(path, "r");
    if (f) {
      char   str[8] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
      size_t n_read = fread(str, 1, 6, f);
      fclose(f);

      if (!!strncmp(str, "Hello\n", n_read)) {
        fprintf(stderr, "error: Restored bad file `%s' != `Hello'\n", str);
      }
    }
    free_path->free_path(free_path->handle, path);
  }

  apath = (char*)retrieve(callback_data,
                          map_uri(plugin, "http://example.org/save-file"),
                          &size,
                          &type,
                          &valflags);
  if (apath) {
    char* spath = map_path->absolute_path(map_path->handle, apath);
    FILE* sfile = fopen(spath, "r");
    if (!sfile) {
      fprintf(stderr, "error: Failed to open save file %s\n", spath);
    } else {
      fclose(sfile);
    }
    free_path->free_path(free_path->handle, spath);
  } else {
    fprintf(stderr, "error: Failed to restore save file.\n");
  }

  return LV2_STATE_SUCCESS;
}

static const void*
extension_data(const char* uri)
{
  static const LV2_State_Interface state = {save, restore};
  if (!strcmp(uri, LV2_STATE__interface)) {
    return &state;
  }
  return NULL;
}

static const LV2_Descriptor descriptor = {TEST_URI,
                                          instantiate,
                                          connect_port,
                                          NULL, // activate,
                                          run,
                                          NULL, // deactivate,
                                          cleanup,
                                          extension_data};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
  return index ? NULL : &descriptor;
}
