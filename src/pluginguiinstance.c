/* SLV2
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <slv2/types.h>
#include <slv2/plugin.h>
#include <slv2/pluginguiinstance.h>
#include <slv2/util.h>
#include "slv2_internal.h"


SLV2GUIInstance
slv2_plugin_gui_instantiate(SLV2Plugin                 plugin,
			    SLV2Value                  gui,
			    LV2UI_Set_Control_Function control_function,
			    LV2UI_Controller           controller,
			    const LV2_Host_Feature**   host_features)
{
        struct _GUIInstance* result = NULL;
	
	bool local_host_features = (host_features == NULL);
	if (local_host_features) {
		host_features = malloc(sizeof(LV2_Host_Feature));
		host_features[0] = NULL;
	}
	
	const char* const lib_uri = slv2_value_as_uri(slv2_plugin_get_gui_library_uri(plugin, gui));
	const char* const lib_path = slv2_uri_to_path(lib_uri);
	
	if (!lib_path)
		return NULL;
	
	dlerror();
	void* lib = dlopen(lib_path, RTLD_NOW);
	if (!lib) {
		fprintf(stderr, "Unable to open GUI library %s (%s)\n", lib_path, dlerror());
		return NULL;
	}

	LV2UI_DescriptorFunction df = dlsym(lib, "lv2ui_descriptor");

	if (!df) {
		fprintf(stderr, "Could not find symbol 'lv2ui_descriptor', "
				"%s is not a LV2 plugin GUI.\n", lib_path);
		dlclose(lib);
		return NULL;
	} else {
		// Search for plugin by URI
		
		// FIXME: Kluge to get bundle path (containing directory of binary)
		char* bundle_path = strdup(plugin->binary_uri);
		char* const bundle_path_end = strrchr(bundle_path, '/');
		if (bundle_path_end)
		*(bundle_path_end+1) = '\0';
		printf("GUI bundle path: %s\n", bundle_path);
		
		for (uint32_t i=0; 1; ++i) {
			
			const LV2UI_Descriptor* ld = df(i);
				
			if (!ld) {
				fprintf(stderr, "Did not find GUI %s in %s\n",
						slv2_value_as_uri(gui), lib_path);
				dlclose(lib);
				break; // return NULL
			} else if (!strcmp(ld->URI, slv2_value_as_uri(gui))) {

				printf("Found GUI %s at index %u in:\n\t%s\n\n",
				       librdf_uri_as_string(plugin->plugin_uri), i, lib_path);

				assert(ld->instantiate);

				// Create SLV2GUIInstance to return
				result = malloc(sizeof(struct _GUIInstance));
				struct _GUIInstanceImpl* impl = malloc(sizeof(struct _GUIInstanceImpl));
				impl->lv2ui_descriptor = ld;
				impl->lv2ui_handle = ld->instantiate(ld, 
								     slv2_plugin_get_uri(plugin),
								     (char*)bundle_path, 
								     control_function,
								     controller,
								     (struct _GtkWidget**)&impl->widget,
								     host_features);
				impl->lib_handle = lib;
				result->pimpl = impl;
				break;
			}
		}

		free(bundle_path);
	}

	assert(result);
	assert(slv2_plugin_get_num_ports(plugin) > 0);

	// Failed to instantiate
	if (result->pimpl->lv2ui_handle == NULL) {
		//printf("Failed to instantiate %s\n", plugin->plugin_uri);
		free(result);
		return NULL;
	}
	
	// Failed to create a widget, but still got a handle - this means that
	// the plugin is buggy
	if (result->pimpl->widget == NULL) {
	  slv2_gui_instance_free(result);
	  return NULL;
	}

	if (local_host_features)
		free(host_features);

	return result;
}


void
slv2_gui_instance_free(SLV2GUIInstance instance)
{
	struct _GUIInstance* i = (struct _GUIInstance*)instance;
	i->pimpl->lv2ui_descriptor->cleanup(i->pimpl->lv2ui_handle);
	i->pimpl->lv2ui_descriptor = NULL;
	dlclose(i->pimpl->lib_handle);
	i->pimpl->lib_handle = NULL;
	free(i->pimpl);
	i->pimpl = NULL;
	free(i);
}


struct _GtkWidget*
slv2_gui_instance_get_widget(SLV2GUIInstance instance) {
  return instance->pimpl->widget;
}


const LV2UI_Descriptor*
slv2_gui_instance_get_descriptor(SLV2GUIInstance instance) {
  return instance->pimpl->lv2ui_descriptor;
}


LV2_Handle
slv2_gui_instance_get_handle(SLV2GUIInstance instance) {
  return instance->pimpl->lv2ui_handle;
}

