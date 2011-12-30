/*
  Lilv Test Plugin
  Copyright 2011 David Robillard <d@drobilla.net>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define TEST_URI "http://example.org/lilv-test-plugin"

#define NS_ATOM "http://lv2plug.in/ns/ext/atom#"

enum {
	TEST_INPUT  = 0,
	TEST_OUTPUT = 1
};

typedef struct {
	LV2_URID_Map* map;

	struct {
		LV2_URID atom_Float;
	} uris;

	float*   input;
	float*   output;
	unsigned num_runs;
} Test;

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Test* test = (Test*)instance;
	switch (port) {
	case TEST_INPUT:
		test->input = (float*)data;
		break;
	case TEST_OUTPUT:
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
	Test* test = (Test*)malloc(sizeof(Test));
	if (!test) {
		return NULL;
	}

	test->map      = NULL;
	test->input    = NULL;
	test->output   = NULL;
	test->num_runs = 0;

	/* Scan host features for URID map */
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
			test->map = (LV2_URID_Map*)features[i]->data;
			test->uris.atom_Float = test->map->map(
				test->map->handle, NS_ATOM "Float");
		}
	}

	if (!test->map) {
		fprintf(stderr, "Host does not support urid:map.\n");
		free(test);
		return NULL;
	}

	return (LV2_Handle)test;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Test* test = (Test*)instance;
	*test->output = *test->input;
	++test->num_runs;
}

static uint32_t
map_uri(Test* plugin, const char* uri)
{
	return plugin->map->map(plugin->map->handle, uri);
}

static void
save(LV2_Handle                instance,
     LV2_State_Store_Function  store,
     void*                     callback_data,
     uint32_t                  flags,
     const LV2_Feature* const* features)
{
	Test* plugin = (Test*)instance;

	store(callback_data,
	      map_uri(plugin, "http://example.org/greeting"),
	      "hello",
	      strlen("hello") + 1,
	      map_uri(plugin, NS_ATOM "String"),
	      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

	const uint32_t urid = map_uri(plugin, "http://example.org/urivalue");
	store(callback_data,
	      map_uri(plugin, "http://example.org/uri"),
	      &urid,
	      sizeof(uint32_t),
	      map_uri(plugin, NS_ATOM "URID"),
	      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

	store(callback_data,
	      map_uri(plugin, "http://example.org/num-runs"),
	      &plugin->num_runs,
	      sizeof(plugin->num_runs),
	      map_uri(plugin, NS_ATOM "Int32"),
	      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

	const float two = 2.0f;
	store(callback_data,
	      map_uri(plugin, "http://example.org/two"),
	      &two,
	      sizeof(two),
	      map_uri(plugin, NS_ATOM "Float"),
	      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

	const uint32_t affirmative = 1;
	store(callback_data,
	      map_uri(plugin, "http://example.org/true"),
	      &affirmative,
	      sizeof(affirmative),
	      map_uri(plugin, NS_ATOM "Bool"),
	      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

	const uint32_t negative = 0;
	store(callback_data,
	      map_uri(plugin, "http://example.org/false"),
	      &negative,
	      sizeof(negative),
	      map_uri(plugin, NS_ATOM "Bool"),
	      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

	const uint8_t blob[] = "This is a bunch of data that happens to be text"
		" but could really be anything at all, lest you feel like cramming"
		" all sorts of ridiculous binary stuff in Turtle";
	store(callback_data,
	      map_uri(plugin, "http://example.org/blob"),
	      blob,
	      sizeof(blob),
	      map_uri(plugin, "http://example.org/SomeUnknownType"),
	      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
}

static void
restore(LV2_Handle                  instance,
        LV2_State_Retrieve_Function retrieve,
        void*                       callback_data,
        uint32_t                    flags,
        const LV2_Feature* const*   features)
{
	Test* plugin = (Test*)instance;

	size_t   size;
	uint32_t type;
	uint32_t valflags;

	plugin->num_runs = *(int32_t*)retrieve(
		callback_data,
		map_uri(plugin, "http://example.org/num-runs"),
		&size, &type, &valflags);
}

const void*
extension_data(const char* uri)
{
	static const LV2_State_Interface state = { save, restore };
	if (!strcmp(uri, LV2_STATE_INTERFACE_URI)) {
		return &state;
	}
	return NULL;
}

static const LV2_Descriptor descriptor = {
	TEST_URI,
	instantiate,
	connect_port,
	NULL, // activate,
	run,
	NULL, // deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
