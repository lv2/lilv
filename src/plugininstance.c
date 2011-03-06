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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slv2_internal.h"

SLV2_API
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

	LV2_Descriptor_Function df = (LV2_Descriptor_Function)
		slv2_dlfunc(lib, "lv2_descriptor");

	if (!df) {
		SLV2_ERRORF("Could not find symbol 'lv2_descriptor', "
		            "%s is not a LV2 plugin.\n", lib_path);
		dlclose(lib);
		return NULL;
	} else {
		// Search for plugin by URI

		const char* bundle_path = slv2_uri_to_path(slv2_value_as_uri(
					slv2_plugin_get_bundle_uri(plugin)));

		for (uint32_t i = 0; true; ++i) {
			const LV2_Descriptor* ld = df(i);
			if (!ld) {
				SLV2_ERRORF("Did not find plugin %s in %s\n",
						slv2_value_as_uri(slv2_plugin_get_uri(plugin)), lib_path);
				dlclose(lib);
				break; // return NULL
			} else {
				// Parse bundle URI to use as base URI
				const SLV2Value bundle_uri     = slv2_plugin_get_bundle_uri(plugin);
				const char*     bundle_uri_str = slv2_value_as_uri(bundle_uri);
				SerdURI         base_uri;
				if (!serd_uri_parse((const uint8_t*)bundle_uri_str, &base_uri)) {
					dlclose(lib);
					break;
				}

				// Resolve library plugin URI against base URI
				SerdURI  abs_uri;
				SerdNode abs_uri_node = serd_node_new_uri_from_string(
					(const uint8_t*)ld->URI, &base_uri, &abs_uri);
				if (!abs_uri_node.buf) {
					SLV2_ERRORF("Failed to parse library plugin URI `%s'\n", ld->URI);
					dlclose(lib);
					break;
				}

				if (!strcmp((const char*)abs_uri_node.buf,
				            slv2_value_as_uri(slv2_plugin_get_uri(plugin)))) {
					// Create SLV2Instance to return
					result = malloc(sizeof(struct _Instance));
					result->lv2_descriptor = ld;
					result->lv2_handle = ld->instantiate(ld, sample_rate, (char*)bundle_path,
							(features) ? features : local_features);
					struct _SLV2InstanceImpl* impl = malloc(sizeof(struct _SLV2InstanceImpl));
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
		for (uint32_t i = 0; i < slv2_plugin_get_num_ports(plugin); ++i)
			result->lv2_descriptor->connect_port(result->lv2_handle, i, NULL);
	}

	free(local_features);

	return result;
}

SLV2_API
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

