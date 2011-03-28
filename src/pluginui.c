/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGE.
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
bool
slv2_ui_supported(SLV2UI    ui,
                  SLV2Value widget_type_uri)
{
#ifdef HAVE_SUIL
	return suil_ui_type_supported(
		slv2_value_as_uri(widget_type_uri),
		slv2_value_as_uri(slv2_values_get_first(ui->classes)));
#else
	return false;
#endif
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
