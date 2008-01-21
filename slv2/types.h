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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define SLV2_NAMESPACE_LV2      "http://lv2plug.in/ns/lv2core#"
#define SLV2_PORT_CLASS_PORT    "http://lv2plug.in/ns/lv2core#Port"
#define SLV2_PORT_CLASS_INPUT   "http://lv2plug.in/ns/lv2core#InputPort"
#define SLV2_PORT_CLASS_OUTPUT  "http://lv2plug.in/ns/lv2core#OutputPort"
#define SLV2_PORT_CLASS_CONTROL "http://lv2plug.in/ns/lv2core#ControlPort"
#define SLV2_PORT_CLASS_AUDIO   "http://lv2plug.in/ns/lv2core#AudioPort"
#define SLV2_PORT_CLASS_MIDI    "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"
#define SLV2_PORT_CLASS_OSC     "http://drobilla.net/ns/lv2ext/osc/0#OSCPort"
#define SLV2_PORT_CLASS_EVENT   "http://lv2plug.in/ns/ext/event#EventPort"

#if 0
/** (Data) Type of a port
 *
 * SLV2_PORT_DATA_TYPE_UNKNOWN means the Port is not of any type SLV2
 * understands.  This does not mean the port is unusable with slv2
 * however:further class information can be using
 * slv2_port_get_value(p, "rdf:type") or a custom query.
 */
typedef enum _SLV2PortDataType {
	SLV2_PORT_DATA_TYPE_UNKNOWN,
	SLV2_PORT_DATA_TYPE_CONTROL, /**< One float per block */
	SLV2_PORT_DATA_TYPE_AUDIO,   /**< One float per frame */
	SLV2_PORT_DATA_TYPE_MIDI,    /**< DEPRECATED: A buffer of MIDI data (LL extension) */
	SLV2_PORT_DATA_TYPE_OSC,     /**< DEPRECATED: A buffer of OSC data (DR extension) */
	SLV2_PORT_DATA_TYPE_EVENT,   /**< Generic event port */
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
#endif


/** The format of a URI string.
 *
 * Full URI: http://example.org/foo
 * QName: lv2:Plugin
 */
typedef enum _SLV2URIType {
	SLV2_URI,
	SLV2_QNAME
} SLV2URIType;


/** A port on a plugin.  Opaque, but valid to compare to NULL. */
typedef struct _SLV2Port* SLV2Port;


/** The port (I/O) signature of a plugin.  Opaque, but valid to compare to NULL. */
typedef struct _SLV2PortSignature* SLV2PortSignature;


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


/** A plugin template (collection of port signatures). */
typedef void* SLV2Template;


/** A collection of typed values. */
typedef void* SLV2Values;


/** A plugin UI */
typedef struct _SLV2UI* SLV2UI;


/** A collection of plugin UIs. */
typedef void* SLV2UIs;


#ifdef __cplusplus
}
#endif


#endif /* __SLV2_TYPES_H__ */

