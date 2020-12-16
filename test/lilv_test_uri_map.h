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

#ifndef LILV_TEST_URI_MAP_H
#define LILV_TEST_URI_MAP_H

#include "../src/lilv_internal.h"

#include "lv2/urid/urid.h"
#include "serd/serd.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	char** uris;
	size_t n_uris;
} LilvTestUriMap;

static inline void
lilv_test_uri_map_init(LilvTestUriMap* const map)
{
	map->uris   = NULL;
	map->n_uris = 0;
}

static inline void
lilv_test_uri_map_clear(LilvTestUriMap* const map)
{
	for (size_t i = 0; i < map->n_uris; ++i) {
		free(map->uris[i]);
	}

	free(map->uris);
	map->uris   = NULL;
	map->n_uris = 0;
}

static inline LV2_URID
map_uri(LV2_URID_Map_Handle handle, const char* uri)
{
	LilvTestUriMap* map = (LilvTestUriMap*)handle;

	for (size_t i = 0; i < map->n_uris; ++i) {
		if (!strcmp(map->uris[i], uri)) {
			return i + 1;
		}
	}

	assert(serd_uri_string_has_scheme((const uint8_t*)uri));

	map->uris = (char**)realloc(map->uris, ++map->n_uris * sizeof(char*));
	map->uris[map->n_uris - 1] = lilv_strdup(uri);
	return map->n_uris;
}

static inline const char*
unmap_uri(LV2_URID_Map_Handle handle, LV2_URID urid)
{
	LilvTestUriMap* map = (LilvTestUriMap*)handle;

	if (urid > 0 && urid <= map->n_uris) {
		return map->uris[urid - 1];
	}

	return NULL;
}

#endif // LILV_TEST_URI_MAP_H
