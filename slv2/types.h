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

#ifndef __SLV2_TYPES_H__
#define __SLV2_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>


/* A property, resulting from a query.
 *
 * Note that properties may have many values.
 */
struct _Property {
	size_t num_values;
	char** values;
};

typedef struct _Property* SLV2Property;


/** Class (direction and rate) of a port */
enum SLV2PortClass {
	SLV2_UNKNOWN_PORT_CLASS,
	SLV2_CONTROL_RATE_INPUT,  /**< One input value per block */
	SLV2_CONTROL_RATE_OUTPUT, /**< One output value per block */
	SLV2_AUDIO_RATE_INPUT,    /**< One input value per frame */
	SLV2_AUDIO_RATE_OUTPUT    /**< One output value per frame */
};


/** lv2:float, IEEE-754 32-bit floating point number */
#define SLV2_DATA_TYPE_FLOAT "http://lv2plug.in/ontology#float"


#ifdef __cplusplus
}
#endif

#endif /* __SLV2_TYPES_H__ */

