/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

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

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "slv2_internal.h"

#ifdef HAVE_SUIL
#include "suil/suil.h"
#endif

SLV2UI
slv2_ui_new(SLV2World world,
            SLV2Value uri,
            SLV2Value type_uri,
            SLV2Value binary_uri)
{
	assert(uri);
	assert(type_uri);
	assert(binary_uri);

	struct _SLV2UI* ui = malloc(sizeof(struct _SLV2UI));
	ui->world      = world;
	ui->uri        = uri;
	ui->binary_uri = binary_uri;

	// FIXME: kludge
	char* bundle     = slv2_strdup(slv2_value_as_string(ui->binary_uri));
	char* last_slash = strrchr(bundle, '/') + 1;
	*last_slash = '\0';
	ui->bundle_uri = slv2_value_new_uri(world, bundle);
	free(bundle);

	ui->classes = slv2_values_new();
	slv2_array_append(ui->classes, type_uri);

	return ui;
}

void
slv2_ui_free(SLV2UI ui)
{
	slv2_value_free(ui->uri);
	ui->uri = NULL;

	slv2_value_free(ui->bundle_uri);
	ui->bundle_uri = NULL;

	slv2_value_free(ui->binary_uri);
	ui->binary_uri = NULL;

	slv2_values_free(ui->classes);

	free(ui);
}

SLV2_API
SLV2Value
slv2_ui_get_uri(SLV2UI ui)
{
	assert(ui);
	assert(ui->uri);
	return ui->uri;
}

SLV2_API
unsigned
slv2_ui_is_supported(SLV2UI              ui,
                     SLV2UISupportedFunc supported_func,
                     SLV2Value           container_type,
                     SLV2Value*          ui_type)
{
#ifdef HAVE_SUIL
	SLV2Values classes = slv2_ui_get_classes(ui);
	SLV2_FOREACH(c, classes) {
		SLV2Value type = slv2_values_get(classes, c);
		const unsigned q = supported_func(slv2_value_as_uri(container_type),
		                                  slv2_value_as_uri(type));
		if (q) {
			if (ui_type) {
				*ui_type = slv2_value_duplicate(type);
			}
			return q;
		}
	}
#endif
	return 0;
}

SLV2_API
SLV2Values
slv2_ui_get_classes(SLV2UI ui)
{
	return ui->classes;
}

SLV2_API
bool
slv2_ui_is_a(SLV2UI ui, SLV2Value ui_class_uri)
{
	return slv2_values_contains(ui->classes, ui_class_uri);
}

SLV2_API
SLV2Value
slv2_ui_get_bundle_uri(SLV2UI ui)
{
	assert(ui);
	assert(ui->bundle_uri);
	return ui->bundle_uri;
}

SLV2_API
SLV2Value
slv2_ui_get_binary_uri(SLV2UI ui)
{
	assert(ui);
	assert(ui->binary_uri);
	return ui->binary_uri;
}
