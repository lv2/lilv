/* LibSLV2
 * Copyright (C) 2006 Dave Robillard <drobilla@connect.carleton.ca>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <slv2/private_types.h>
#include <slv2/types.h>
#include <slv2/plugin.h>
#include <slv2/plugininstance.h>
#include "util.h"


SLV2Instance*
slv2_plugin_instantiate(const SLV2Plugin*        plugin,
                        uint32_t                 sample_rate,
                        const LV2_Host_Feature** host_features)
{
	struct _Instance* result = NULL;
	
	bool local_host_features = (host_features == NULL);
	if (local_host_features) {
		host_features = malloc(sizeof(LV2_Host_Feature));
		host_features[0] = NULL;
	}
	
	const char* const lib_path = slv2_plugin_get_library_path(plugin);
	if (!lib_path)
		return NULL;
	
	dlerror();
	void* lib = dlopen((char*)lib_path, RTLD_NOW);
	if (!lib) {
		fprintf(stderr, "Unable to open library %s (%s)\n", lib_path, dlerror());
		return NULL;
	}

	LV2_Descriptor_Function df = dlsym(lib, "lv2_descriptor");

	if (!df) {
		fprintf(stderr, "Could not find symbol 'lv2_descriptor', "
				"%s is not a LV2 plugin.\n", lib_path);
		dlclose(lib);
		return NULL;
	} else {
		// Search for plugin by URI
		
		const char* const bundle_path = url2path(plugin->bundle_url);
		
		for (uint32_t i=0; 1; ++i) {
			const LV2_Descriptor* ld = df(i);

			if (!ld) {
				fprintf(stderr, "Did not find plugin %s in %s\n",
						plugin->plugin_uri, plugin->lib_url);
				dlclose(lib);
				break; // return NULL
			} else if (!strcmp(ld->URI, (char*)plugin->plugin_uri)) {
				//printf("Found %s at index %u in:\n\t%s\n\n", plugin->plugin_uri, i, lib_path);

				assert(ld->instantiate);

				// Create SLV2Instance to return
				result = malloc(sizeof(struct _Instance));
				/*result->plugin = malloc(sizeof(struct _Plugin));
				memcpy(result->plugin, plugin, sizeof(struct _Plugin));*/
				result->descriptor = ld;
				result->lib_handle = lib;
				result->lv2_handle = ld->instantiate(ld, sample_rate, (char*)bundle_path, host_features);
				break;
			}
		}
	}

	assert(result);
	assert(slv2_plugin_get_num_ports(plugin) > 0);

	// Failed to instantiate
	if (result->lv2_handle == NULL) {
		//printf("Failed to instantiate %s\n", plugin->plugin_uri);
		free(result);
		return NULL;
	}

	// "Connect" all ports to NULL (catches bugs)
	for (uint32_t i=0; i < slv2_plugin_get_num_ports(plugin); ++i)
		result->descriptor->connect_port(result->lv2_handle, i, NULL);
	
	if (local_host_features)
		free(host_features);

	return result;
}


void
slv2_instance_free(SLV2Instance* instance)
{
	struct _Instance* i = (struct _Instance*)instance;
	i->descriptor->cleanup(i->lv2_handle);
	i->descriptor = NULL;
	dlclose(i->lib_handle);
	i->lib_handle = NULL;
	free(i);
}


