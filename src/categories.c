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

#define _XOPEN_SOURCE 500

#include <string.h>
#include <limits.h>
#include <librdf.h>
#include <slv2/category.h>
#include <slv2/categories.h>
#include "private_types.h"

	
SLV2Categories
slv2_categories_new()
{
	return raptor_new_sequence((void (*)(void*))&slv2_category_free, NULL);
}


void
slv2_categories_free(SLV2Categories list)
{
	//if (list != world->categories)
		raptor_free_sequence(list);
}


unsigned
slv2_categories_size(SLV2Categories list)
{
	return raptor_sequence_size(list);
}


SLV2Category
slv2_categories_get_by_uri(SLV2Categories list, const char* uri)
{
	// good old fashioned binary search
	
	int lower = 0;
	int upper = raptor_sequence_size(list) - 1;
	int i;
	
	if (upper == 0)
		return NULL;

	while (upper >= lower) {
		i = lower + ((upper - lower) / 2);

		SLV2Category p = raptor_sequence_get_at(list, i);

		int cmp = strcmp(slv2_category_get_uri(p), uri);

		if (cmp == 0)
			return p;
		else if (cmp > 0)
			upper = i - 1;
		else
			lower = i + 1;
	}

	return NULL;
}


SLV2Category
slv2_categories_get_at(SLV2Categories list, unsigned index)
{	
	if (index > INT_MAX)
		return NULL;
	else
		return (SLV2Category)raptor_sequence_get_at(list, (int)index);
}

