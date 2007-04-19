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

#ifndef __SLV2_PRIVATE_TYPES_H__
#define __SLV2_PRIVATE_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <librdf.h>
#include <slv2/pluginlist.h>


/** Reference to a port on some plugin.
 */
struct _Port {
	uint32_t index;   ///< LV2 index
	char*    symbol;  ///< LV2 symbol
	//char*    node_id; ///< RDF Node ID
};

SLV2Port slv2_port_new(uint32_t index, const char* symbol/*, const char* node_id*/);
SLV2Port slv2_port_duplicate(SLV2Port port);
void     slv2_port_free(SLV2Port port);


/** Record of an installed/available plugin.
 *
 * A simple reference to a plugin somewhere on the system. This just holds
 * paths of relevant files, the actual data therein isn't loaded into memory.
 */
struct _Plugin {
	int              deletable;
	struct _Model*   model;
	librdf_uri*      plugin_uri;
//	char*            bundle_url; // Bundle directory plugin was loaded from
	char*            binary_uri; // lv2:binary
	raptor_sequence* data_uris;  // rdfs::seeAlso
	raptor_sequence* ports;
	librdf_storage*  storage;
	librdf_model*    rdf;
};

SLV2Plugin slv2_plugin_new(SLV2Model model, librdf_uri* uri, const char* binary_uri);
void       slv2_plugin_load(SLV2Plugin p);


/** List of references to plugins available for loading */
struct _PluginList {
	size_t           num_plugins;
	struct _Plugin** plugins;
};

/** Pimpl portion of SLV2Instance */
struct _InstanceImpl {
	void* lib_handle;
};


/** Model of LV2 (RDF) data loaded from bundles.
 */
struct _Model {
	librdf_world*       world;
	librdf_storage*     storage;
	librdf_model*       model;
	librdf_parser*      parser;
	SLV2Plugins plugins;
};


#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PRIVATE_TYPES_H__ */

