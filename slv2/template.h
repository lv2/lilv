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

#ifndef __SLV2_TEMPLATE_H__
#define __SLV2_TEMPLATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <slv2/types.h>

/** \addtogroup slv2_data
 * @{
 */

	
/** Free an SLV2Template.
 */
void
slv2_template_free(SLV2Template);


/** Get the signature (direction and type) of a port
 */
SLV2PortSignature
slv2_template_get_port(SLV2Template t,
                       uint32_t     index);


/** Get the total number of ports.
 */
uint32_t
slv2_template_get_num_ports(SLV2Template t);


/** Get the number of ports of a given direction and type.
 */
uint32_t
slv2_template_get_num_ports_of_type(SLV2Template      t,
                                    SLV2PortDirection direction,
                                    SLV2PortDataType  type);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_TEMPLATE_H__ */

