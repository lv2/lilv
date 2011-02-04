/* SLV2
 * Copyright (C) 2007-2011 David Robillard <http://drobilla.net>
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
#include "slv2/plugin.h"
#include "slv2/plugininstance.h"
#include "slv2/types.h"
#include "slv2/util.h"
#include "slv2/value.h"
#include "slv2_internal.h"

SLV2Instance
slv2_plugin_instantiate(SLV2Plugin               plugin,
                        double                   sample_rate,
                        const LV2_Feature*const* features)
{
	struct _Instance* result = NULL;

	const LV2_Feature** local_features = NULL;
	if (features == NULL) {
		local_features = malloc(sizeof(LV2_Feature));
		local_features[0] = NULL;
	}

	const char* const lib_uri  = slv2_value_as_uri(slv2_plugin_get_library_uri(plugin));
	const char* const lib_path = slv2_uri_to_path(lib_uri);

	if (!lib_path)
		return NULL;

	dlerror();
	void* lib = dlopen(lib_path, RTLD_NOW);
	if (!lib) {
		SLV2_ERRORF("Unable to open library %s (%s)\n", lib_path, dlerror());
		return NULL;
	}

	LV2_Descriptor_Function df = dlsym(lib, "lv2_descriptor");

	if (!df) {
		SLV2_ERRORF("Could not find symbol 'lv2_descriptor', "
				"%s is not a LV2 plugin.\n", lib_path);
		dlclose(lib);
		return NULL;
	} else {
		// Search for plugin by URI

		const char* bundle_path = slv2_uri_to_path(slv2_value_as_uri(
					slv2_plugin_get_bundle_uri(plugin)));

		for (uint32_t i=0; 1; ++i) {

			const LV2_Descriptor* ld = df(i);

			if (!ld) {
				SLV2_ERRORF("Did not find plugin %s in %s\n",
						slv2_value_as_uri(slv2_plugin_get_uri(plugin)), lib_path);
				dlclose(lib);
				break; // return NULL
			} else {
				// FIXME: duplicated/common code, put this in serd
				SerdURI uri;
				if (!serd_uri_parse((const uint8_t*)ld->URI, &uri)) {
					SLV2_ERROR("Failed to parse library URI\n");
					dlclose(lib);
					break;
				}

				SerdURI base_uri;
				if (!serd_uri_parse(
					    (const uint8_t*)slv2_value_as_uri(slv2_plugin_get_bundle_uri(plugin)),
					    &base_uri)) {
					dlclose(lib);
					break;
				}

				SerdURI abs_uri;
				if (!serd_uri_resolve(&uri, &base_uri, &abs_uri)) {
					fprintf(stderr, "error: failed to resolve new base URI\n");
					return false;
				}

				SerdNode abs_uri_node = serd_node_new_uri(&abs_uri, &base_uri);

				if (!strcmp((const char*)abs_uri_node.buf,
				            slv2_value_as_uri(slv2_plugin_get_uri(plugin)))) {
					assert(plugin->plugin_uri);
					assert(ld->instantiate);

					// Create SLV2Instance to return
					result = malloc(sizeof(struct _Instance));
					result->lv2_descriptor = ld;
					result->lv2_handle = ld->instantiate(ld, sample_rate, (char*)bundle_path,
							(features) ? features : local_features);
					struct _InstanceImpl* impl = malloc(sizeof(struct _InstanceImpl));
					impl->lib_handle = lib;
					result->pimpl = impl;

					serd_node_free(&abs_uri_node);
					break;
				} else {
					serd_node_free(&abs_uri_node);
				}
			}
		}
	}

	if (result) {
		assert(slv2_plugin_get_num_ports(plugin) > 0);

		// Failed to instantiate
		if (result->lv2_handle == NULL) {
			free(result);
			return NULL;
		}

		// "Connect" all ports to NULL (catches bugs)
		for (uint32_t i=0; i < slv2_plugin_get_num_ports(plugin); ++i)
			result->lv2_descriptor->connect_port(result->lv2_handle, i, NULL);
	}

	free(local_features);

	return result;
}

void
slv2_instance_free(SLV2Instance instance)
{
	if (!instance)
		return;

	struct _Instance* i = (struct _Instance*)instance;
	i->lv2_descriptor->cleanup(i->lv2_handle);
	i->lv2_descriptor = NULL;
	dlclose(i->pimpl->lib_handle);
	i->pimpl->lib_handle = NULL;
	free(i->pimpl);
	i->pimpl = NULL;
	free(i);
}

