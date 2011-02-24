/* SLV2
 * Copyright (C) 2007-2011 David Robillard <http://drobilla.net>
 * Author: Lars Luthman
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
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
	return slv2_ui_instance_new(
		plugin, ui, NULL, write_function, controller, features);
}

SLV2_API
SLV2UIInstance
slv2_ui_instance_new(SLV2Plugin                plugin,
                     SLV2UI                    ui,
                     SLV2Value                 widget_type_uri,
                     LV2UI_Write_Function      write_function,
                     LV2UI_Controller          controller,
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
		write_function,
		controller,
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

