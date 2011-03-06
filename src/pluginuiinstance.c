/*
  Copyright 2011 David Robillard <http://drobilla.net>

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SUIL
#include "suil/suil.h"
#endif

#include "slv2_internal.h"

SLV2_DEPRECATED
SLV2_API
SLV2UIInstance
slv2_ui_instantiate(SLV2Plugin                plugin,
                    SLV2UI                    ui,
                    LV2UI_Write_Function      write_function,
                    LV2UI_Controller          controller,
                    const LV2_Feature* const* features)
{
	SLV2UIHost ui_host = slv2_ui_host_new(
		controller, write_function, NULL, NULL, NULL);
	
	SLV2UIInstance ret = slv2_ui_instance_new(
		plugin, ui, NULL, ui_host, features);

	slv2_ui_host_free(ui_host);
	return ret;
}

SLV2_API
SLV2UIHost
slv2_ui_host_new(LV2UI_Controller            controller,
                 LV2UI_Write_Function        write_function,
                 SLV2PortIndexFunction       port_index_function,
                 SLV2PortSubscribeFunction   port_subscribe_function,
                 SLV2PortUnsubscribeFunction port_unsubscribe_function)
{
	SLV2UIHost ret = malloc(sizeof(struct _SLV2UIHost));
	ret->controller                = controller;
	ret->write_function            = write_function;
	ret->port_index_function       = port_index_function;
	ret->port_subscribe_function   = port_subscribe_function;
	ret->port_unsubscribe_function = port_unsubscribe_function;
	return ret;
}

SLV2_API
void
slv2_ui_host_free(SLV2UIHost ui_host)
{
	free(ui_host);
}
	
SLV2_API
SLV2UIInstance
slv2_ui_instance_new(SLV2Plugin                plugin,
                     SLV2UI                    ui,
                     SLV2Value                 widget_type_uri,
                     SLV2UIHost                ui_host,
                     const LV2_Feature* const* features)
{
#ifdef HAVE_SUIL
	const char* const bundle_uri  = slv2_value_as_uri(slv2_ui_get_bundle_uri(ui));
	const char* const bundle_path = slv2_uri_to_path(bundle_uri);
	const char* const lib_uri     = slv2_value_as_string(slv2_ui_get_binary_uri(ui));
	const char* const lib_path    = slv2_uri_to_path(lib_uri);
	if (!bundle_path || !lib_path) {
		return NULL;
	}

	SLV2Value ui_type = slv2_values_get_at(ui->classes, 0);
	if (!widget_type_uri) {
		widget_type_uri = ui_type;
	}

	SuilInstance suil_instance = suil_instance_new(
		slv2_value_as_uri(slv2_plugin_get_uri(plugin)),
		slv2_value_as_uri(slv2_ui_get_uri(ui)),
		bundle_path,
		lib_path,
		slv2_value_as_uri(ui_type),
		slv2_value_as_uri(widget_type_uri),
		ui_host->write_function,
		ui_host->controller,
		features);

	if (!suil_instance) {
		return NULL;
	}

	// Create SLV2UIInstance to return
	struct _SLV2UIInstance* result = malloc(sizeof(struct _SLV2UIInstance));
	result->instance = suil_instance;

	return result;
#else
	return NULL;
#endif
}

SLV2_API
void
slv2_ui_instance_free(SLV2UIInstance instance)
{
#ifdef HAVE_SUIL
	if (instance) {
		suil_instance_free(instance->instance);
		free(instance);
	}
#else
	return NULL;
#endif
}

SLV2_API
LV2UI_Widget
slv2_ui_instance_get_widget(SLV2UIInstance instance)
{
#ifdef HAVE_SUIL
	return suil_instance_get_widget(instance->instance);
#else
	return NULL;
#endif
}

SLV2_API
void
slv2_ui_instance_port_event(SLV2UIInstance instance,
                            uint32_t       port_index,
                            uint32_t       buffer_size,
                            uint32_t       format,
                            const void*    buffer)
{
	suil_instance_port_event(instance->instance,
	                         port_index,
	                         buffer_size,
	                         format,
	                         buffer);
}

SLV2_API
const void*
slv2_ui_instance_extension_data(SLV2UIInstance instance,
                                const char*    uri)
{
#ifdef HAVE_SUIL
	return suil_instance_extension_data(instance->instance, uri);
#else
	return NULL;
#endif
}

SLV2_API
const LV2UI_Descriptor*
slv2_ui_instance_get_descriptor(SLV2UIInstance instance)
{
#ifdef HAVE_SUIL
	return suil_instance_get_descriptor(instance->instance);
#else
	return NULL;
#endif
}

SLV2_API
LV2UI_Handle
slv2_ui_instance_get_handle(SLV2UIInstance instance)
{
#ifdef HAVE_SUIL
	return suil_instance_get_handle(instance->instance);
#else
	return NULL;
#endif
}

