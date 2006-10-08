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

#ifndef __SLV2_PRIVATE_TYPES_H__
#define __SLV2_PRIVATE_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <slv2/lv2.h>


/* If you're a user of SLV2, stop reading this file RIGHT NOW.
 * Unfortunately it needs to be exposed to allow inlining of some things that
 * really need to be inlined, but these are opaque types.  Don't even think
 * about writing code that depends on any information here :)
 */


/** Record of an installed/available plugin.
 *
 * A simple reference to a plugin somewhere on the system. This just holds
 * paths of relevant files, the actual data therein isn't loaded into memory.
 */
struct _Plugin {
	char* plugin_uri;
	char* bundle_url; // Bundle directory plugin was loaded from
	char* data_url;   // rdfs::seeAlso
	char* lib_url;    // lv2:binary
};


/** Instance of a plugin (private type) */
struct _Instance {
	// FIXME: copy plugin here for convenience?
	//struct LV2Plugin*   plugin;
	const LV2_Descriptor* descriptor;
	void*                 lib_handle;
	LV2_Handle            lv2_handle;
};


/** List of references to plugins available for loading (private type) */
struct _PluginList {
	size_t           num_plugins;
	struct _Plugin** plugins;
};


#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PRIVATE_TYPES_H__ */

