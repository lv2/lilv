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

#ifndef __SLV2_PORT_H__
#define __SLV2_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "plugin.h"

/** \addtogroup data
 * @{
 */


/** Get a property of a port, by port index.
 *
 * Return value must be freed by caller with slv2_property_free.
 */
SLV2Property
slv2_port_get_property(SLV2Plugin*   plugin,
                       uint32_t      index,
                       const char*   property);


/** Get the symbol of a port given the index.
 *
 * The 'symbol' is a short string, a valid C identifier.
 * Returned string must be free()'d by caller.
 *
 * \return NULL when index is out of range
 */
char*
slv2_port_get_symbol(SLV2Plugin*   plugin,
                     uint32_t      index);


/** Get the class (direction and rate) of a port.
 */
enum SLV2PortClass
slv2_port_get_class(SLV2Plugin*   plugin,
                    uint32_t      index);


/** Get the data type of a port (as a URI).
 *
 * The only data type included in the core LV2 specification is lv2:float.
 * Compare this return value with SLV2_DATA_TYPE_FLOAT to check for it.
 *
 * Returned string must be free()'d by caller.
 */
char*
slv2_port_get_data_type(SLV2Plugin*   plugin,
                        uint32_t      index);


/** Get the default value of a port.
 *
 * Only valid for ports with a data type of lv2:float.
 */
float
slv2_port_get_default_value(SLV2Plugin*   plugin, 
                            uint32_t      index);


/** Get the minimum value of a port.
 *
 * Only valid for ports with a data type of lv2:float.
 */
float
slv2_port_get_minimum_value(SLV2Plugin*   plugin, 
                            uint32_t      index);


/** Get the maximum value of a port.
 *
 * Only valid for ports with a data type of lv2:float.
 */
float
slv2_port_get_maximum_value(SLV2Plugin*   plugin, 
                            uint32_t      index);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PORT_H__ */
