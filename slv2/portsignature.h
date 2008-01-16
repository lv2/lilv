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

#ifndef __SLV2_PORTSIGNATURE_H__
#define __SLV2_PORTSIGNATURE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <slv2/types.h>

/** \addtogroup slv2_data
 * @{
 */


/** Get the direction (input or output) of the port.
 *
 * Time = O(1)
 */
SLV2PortDirection
slv2_port_signature_get_direction(SLV2PortSignature sig);


/** Get the type (e.g. audio, midi) of the port.
 *
 * Time = O(1)
 */
SLV2PortDataType
slv2_port_signature_get_type(SLV2PortSignature sig);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PORTSIGNATURE_H__ */

