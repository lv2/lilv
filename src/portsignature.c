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

#include <stdlib.h>
#include <slv2/types.h>
#include "slv2_internal.h"


/* private */
SLV2PortSignature
slv2_port_signature_new(SLV2PortDirection direction,
                        SLV2PortDataType  type)
{
	struct _SLV2PortSignature* ret = malloc(sizeof(struct _SLV2PortSignature));
	ret->direction = direction;
	ret->type = type;
	return ret;
}


/* private */
void
slv2_port_signature_free(SLV2PortSignature sig)
{
	free(sig);
}


SLV2PortDirection
slv2_port_signature_get_direction(SLV2PortSignature sig)
{
	return sig->direction;
}


SLV2PortDataType
slv2_port_signature_get_type(SLV2PortSignature sig)
{
	return sig->type;
}

