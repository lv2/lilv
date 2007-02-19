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

#include <slv2/types.h>
#include <slv2/plugin.h>
#include <slv2/stringlist.h>

/** \addtogroup data
 * @{
 */


/** Create an ID to reference a port by index.
 */
SLV2PortID
slv2_port_by_index(uint32_t index);


/** Create an ID to reference a port by symbol.
 */
SLV2PortID
slv2_port_by_symbol(const char* symbol);


/** Port equivalent to slv2_plugin_get_value.
 */
SLV2Strings
slv2_port_get_value(SLV2Plugin  plugin,
                    SLV2PortID  id,
                    const char* property);


/** Port equivalent to slv2_plugin_get_properties.
 */
SLV2Strings
slv2_port_get_properties(SLV2Plugin p,
                         SLV2PortID        id);


/** Port equivalent to slv2_plugin_get_hints.
 */
SLV2Strings
slv2_port_get_hints(SLV2Plugin p,
                    SLV2PortID        id);


/** Get the symbol of a port given the index.
 *
 * The 'symbol' is a short string, a valid C identifier.
 * Returned string must be free()'d by caller.
 *
 * \return NULL when index is out of range
 */
char*
slv2_port_get_symbol(SLV2Plugin plugin,
                     SLV2PortID  id);

/** Get the name of a port.
 *
 * This is guaranteed to return the untranslated name (the doap:name in the
 * data file without a language tag).  Returned value must be free()'d by
 * the caller.
 */
char*
slv2_port_get_name(SLV2Plugin plugin,
                   SLV2PortID  id);


/** Get the class (input/output, data type, rate...) of a port.
 */
SLV2PortClass
slv2_port_get_class(SLV2Plugin plugin,
                    SLV2PortID  id);


/** Get the default value of a port.
 *
 * Only valid for ports with a data type of lv2:float.
 */
float
slv2_port_get_default_value(SLV2Plugin plugin, 
                            SLV2PortID  id);


/** Get the minimum value of a port.
 *
 * Only valid for ports with a data type of lv2:float.
 */
float
slv2_port_get_minimum_value(SLV2Plugin plugin, 
                            SLV2PortID  id);


/** Get the maximum value of a port.
 *
 * Only valid for ports with a data type of lv2:float.
 */
float
slv2_port_get_maximum_value(SLV2Plugin plugin, 
                            SLV2PortID  id);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PORT_H__ */
