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

#ifndef __SLV2_TYPES_H__
#define __SLV2_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/** (Data) Type of a port
 *
 * SLV2_UNKNOWN_PORT_TYPE means the Port is not of any type SLV2 understands
 * (currently Control, Audio, MIDI, and OSC).
 *
 * Further class information can be using slv2_port_get_value(p, "rdf:type")
 * or a custom query.
 */
typedef enum _SLV2PortDataType {
	SLV2_PORT_DATA_TYPE_UNKNOWN,
	SLV2_PORT_DATA_TYPE_CONTROL, /**< One float per block */
	SLV2_PORT_DATA_TYPE_AUDIO,   /**< One float per frame */
	SLV2_PORT_DATA_TYPE_MIDI,    /**< A buffer of MIDI data (LL extension) */
	SLV2_PORT_DATA_TYPE_OSC,     /**< A buffer of OSC data (DR extension) */
} SLV2PortDataType;

/** Direction (input or output) of a port
 *
 * SLV2_UNKNOWN_PORT_DIRECTION means the Port is only of type lv2:Port
 * (neither lv2:Input or lv2:Output) as far as SLV2 understands.
 *
 * Further class information can be using slv2_port_get_value(p, "rdf:type")
 * or a custom query.
 */
typedef enum _SLV2PortDirection {
	SLV2_PORT_DIRECTION_UNKNOWN, /**< Neither input or output */
	SLV2_PORT_DIRECTION_INPUT,   /**< Plugin reads from port when run */
	SLV2_PORT_DIRECTION_OUTPUT,  /**< Plugin writes to port when run */
} SLV2PortDirection;


/** The format of a URI string.
 *
 * Full URI: http://example.org/foo
 * QName: lv2:Plugin
 */
typedef enum _SLV2URIType {
	SLV2_URI,
	SLV2_QNAME
} SLV2URIType;


/** A type of plugin UI (corresponding to some LV2 UI extension).
 */
typedef enum _SLV2UIType {
	SLV2_UI_TYPE_GTK2 ///< http://ll-plugins.nongnu.org/lv2/ext/gui/dev/1#GtkGUI
} SLV2UIType;


/** A port on a plugin.  Opaque, but valid to compare to NULL. */
typedef struct _SLV2Port* SLV2Port;


/** A plugin.  Opaque, but valid to compare to NULL. */
typedef struct _SLV2Plugin* SLV2Plugin;


/** A collection of plugins.  Opaque, but valid to compare to NULL. */
typedef void* SLV2Plugins;


/** The world.  Opaque, but valid to compare to NULL. */
typedef struct _SLV2World* SLV2World;


/** A plugin class.  Opaque, but valid to compare to NULL. */
typedef struct _SLV2PluginClass* SLV2PluginClass;


/** A collection of plugin classes.  Opaque, but valid to compare to NULL. */
typedef void* SLV2PluginClasses;


/** A typed value */
typedef struct _SLV2Value* SLV2Value;


/** A collection of typed values. */
typedef void* SLV2Values;


/** A plugin UI */
typedef void* SLV2UI;


#ifdef __cplusplus
}
#endif


#endif /* __SLV2_TYPES_H__ */

