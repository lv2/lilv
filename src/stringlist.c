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

#include <string.h>
#include <stdlib.h>
#include <raptor.h>
#include <slv2/stringlist.h>


SLV2Strings
slv2_strings_new()
{
	return raptor_new_sequence(&free, NULL);
}


int
slv2_strings_size(const SLV2Strings list)
{
	return raptor_sequence_size(list);
}


char*
slv2_strings_get_at(const SLV2Strings list, int index)
{
	return (char*)raptor_sequence_get_at(list, index);
}


bool
slv2_strings_contains(const SLV2Strings list, const char* uri)
{
	for (int i=0; i < slv2_strings_size(list); ++i)
		if (!strcmp(slv2_strings_get_at(list, i), uri))
			return true;
	
	return false;
}
