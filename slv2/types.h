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

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <slv2/private_types.h>

#ifdef __cplusplus
extern "C" {
#endif


/* A property, resulting from a query.
 *
 * Note that properties may have many values.
 */
struct _Value {
	size_t num_values;
	char** values;
};

typedef struct _Value* SLV2Value;


/** Free an SLV2Value. */
void
slv2_value_free(SLV2Value);


/** Port ID type, to allow passing either symbol or index
 * to port related functions.
 */
typedef struct _PortID {
	bool        is_index; /**< Otherwise, symbol */
	uint32_t    index;
	const char* symbol;
} SLV2PortID;


/** Class (direction and type) of a port
 *
 * Note that ports may be of other classes not listed here, this is just
 * to make the most common case simple.  Use slv2_port_get_value(p, "rdf:type")
 * if you need further class information.
 */
typedef enum _PortClass {
	SLV2_UNKNOWN_PORT_CLASS,
	SLV2_CONTROL_INPUT,  /**< One input float per block */
	SLV2_CONTROL_OUTPUT, /**< One output float per block */
	SLV2_AUDIO_INPUT,    /**< One input float per frame */
	SLV2_AUDIO_OUTPUT,   /**< One output float per frame */
	SLV2_MIDI_INPUT,     /**< MIDI input (LL extension) */
	SLV2_MIDI_OUTPUT     /**< MIDI output (LL extension) */
} SLV2PortClass;


/** Get the number of elements in a URI list.
 */
int
slv2_uri_list_size(const SLV2URIList list);


/** Get a URI from a URI list at the given index.
 *
 * @return the element at @index, or NULL if index is out of range.
 */
char*
slv2_uri_list_get_at(const SLV2URIList list, int index);


/** Return whether @list contains @uri.
 */
bool
slv2_uri_list_contains(const SLV2URIList list, const char* uri);

#ifdef __cplusplus
}
#endif


#endif /* __SLV2_TYPES_H__ */

