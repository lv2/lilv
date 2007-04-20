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

#include <stdlib.h>
#include <string.h>
#include <slv2/category.h>
#include "private_types.h"


SLV2Category
slv2_category_new(const char* uri, const char* label)
{
	SLV2Category category = (SLV2Category)malloc(sizeof(struct _Category));
	category->uri = strdup(uri);
	category->label = strdup(label);
	return category;
}


void
slv2_category_free(SLV2Category category)
{
	free(category->uri);
	free(category->label);
	free(category);
}


const char*
slv2_category_get_uri(SLV2Category category)
{
	return category->uri;
}


const char*
slv2_category_get_label(SLV2Category category)
{
	return category->label;
}
