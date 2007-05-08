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


/** Class (direction and type) of a port
 *
 * Note that ports may be of other classes not listed here, this is just
 * to make the most common case simple.  Use slv2_port_get_value(p, "rdf:type")
 * if you need further class information.
 */
typedef enum _SLV2PortClass {
	SLV2_UNKNOWN_PORT_CLASS,
	SLV2_CONTROL_INPUT,  /**< One input float per block */
	SLV2_CONTROL_OUTPUT, /**< One output float per block */
	SLV2_AUDIO_INPUT,    /**< One input float per frame */
	SLV2_AUDIO_OUTPUT,   /**< One output float per frame */
	SLV2_MIDI_INPUT,     /**< MIDI input (LL extension) */
	SLV2_MIDI_OUTPUT     /**< MIDI output (LL extension) */
} SLV2PortClass;


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


#ifdef __cplusplus
}
#endif


#endif /* __SLV2_TYPES_H__ */

