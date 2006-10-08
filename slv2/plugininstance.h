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

#ifndef __SLV2_PLUGININSTANCE_H__
#define __SLV2_PLUGININSTANCE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <dlfcn.h>
#include <slv2/lv2.h>
#include <slv2/private_types.h>
#include <slv2/plugininstance.h>
#include <slv2/plugin.h>
#include <slv2/port.h>


typedef const struct _Instance SLV2Instance;


/** \defgroup lib Plugin library access
 *
 * An SLV2Instance is an instantiated SLV2Plugin (eg a loaded dynamic
 * library).  These functions interact with the binary library code only,
 * they do not read any RDF data files whatsoever.
 * 
 * @{
 */


/** Instantiate a plugin.
 *
 * The returned object represents shared library objects loaded into memory,
 * it must be cleaned up with slv2instance_free when no longer
 * needed.
 * 
 * \a plugin is not modified or directly referenced by the returned object
 * (instances store only a copy of the plugin's URI).
 * 
 * \return NULL if instantiation failed.
 */
SLV2Instance*
slv2_plugin_instantiate(const SLV2Plugin*        plugin,
                        uint32_t                 sample_rate,
                        const LV2_Host_Feature** host_features);


/** Free a plugin instance.
 *
 * \a instance is invalid after this call.
 */
void
slv2_instance_free(SLV2Instance* instance);


#ifndef LIBSLV2_SOURCE


/** Get the URI of the plugin which \a instance is an instance of.
 *
 * Returned string is shared and must not be modified or deleted.
 */
inline const char*
slv2_instance_get_uri(SLV2Instance* instance)
{
	assert(instance);
	assert(instance->descriptor);
	
	return instance->descriptor->URI;
}


/** Connect a port to a data location.
 *
 * This may be called regardless of whether the plugin is activated,
 * activation and deactivation does not destroy port connections.
 */
inline void
slv2_instance_connect_port(SLV2Instance* instance,
                           uint32_t      port_index,
                           void*         data_location)
{
	assert(instance);
	assert(instance->descriptor);
	assert(instance->descriptor->connect_port);
	
	instance->descriptor->connect_port
		(instance->lv2_handle, port_index, data_location);
}


/** Activate a plugin instance.
 *
 * This resets all state information in the plugin, except for port data
 * locations (as set by slv2instance_connect_port).  This MUST be called
 * before calling slv2instance_run.
 */
inline void
slv2_instance_activate(SLV2Instance* instance)
{
	assert(instance);
	assert(instance->descriptor);
	
	if (instance->descriptor->activate)
		instance->descriptor->activate(instance->lv2_handle);
}


/** Run \a instance for \a sample_count frames.
 *
 * If the hint lv2:realtimeSafe is set for this plugin, this function is
 * guaranteed not to block.
 */
inline void
slv2_instance_run(SLV2Instance* instance,
                  uint32_t      sample_count)
{
	assert(instance);
	assert(instance->descriptor);
	assert(instance->lv2_handle),
	assert(instance->descriptor->run);
	
	instance->descriptor->run(instance->lv2_handle, sample_count);
}


/** Deactivate a plugin instance.
 *
 * Note that to run the plugin after this you must activate it, which will
 * reset all state information (except port connections).
 */
inline void
slv2_instance_deactivate(SLV2Instance* instance)
{
	assert(instance);
	assert(instance->descriptor);
	assert(instance->lv2_handle);
	
	if (instance->descriptor->deactivate)
		instance->descriptor->deactivate(instance->lv2_handle);
}


/** Get the LV2_Descriptor of the plugin instance.
 *
 * Normally hosts should not need to access the LV2_Descriptor directly,
 * use the slv2instance_* functions.
 *
 * The returned descriptor is shared and must not be deleted.
 */
inline const LV2_Descriptor*
slv2_instance_get_descriptor(SLV2Instance* instance)
{
	assert(instance);
	assert(instance->descriptor);
	
	return instance->descriptor;
}


/** Get the LV2_Handle of the plugin instance.
 *
 * Normally hosts should not need to access the LV2_Handle directly,
 * use the slv2instance_* functions.
 * 
 * The returned handle is shared and must not be deleted.
 */
inline LV2_Handle
slv2_instance_get_handle(SLV2Instance* instance)
{
	assert(instance);
	assert(instance->descriptor);
	
	return instance->lv2_handle;
}

#endif /* LIBSLV2_SOURCE */


/** @} */

#ifdef __cplusplus
}
#endif


#endif /* __SLV2_PLUGININSTANCE_H__ */

