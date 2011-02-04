/* SLV2
 * Copyright (C) 2007-2011 David Robillard <http://drobilla.net>
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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "slv2/types.h"
#include "slv2/plugin.h"
#include "slv2/collections.h"
#include "slv2/util.h"
#include "slv2_internal.h"

SLV2Plugins
slv2_plugins_new()
{
	return g_ptr_array_new();
}

void
slv2_plugins_free(SLV2World world, SLV2Plugins list)
{
	if (list && list != world->plugins)
		g_ptr_array_unref(list);
}

unsigned
slv2_plugins_size(SLV2Plugins list)
{
	return (list ? ((GPtrArray*)list)->len : 0);
}

SLV2Plugin
slv2_plugins_get_by_uri(SLV2Plugins list, SLV2Value uri)
{
	// good old fashioned binary search

	int lower = 0;
	int upper = ((GPtrArray*)list)->len - 1;
	int i;

	while (upper >= lower) {
		i = lower + ((upper - lower) / 2);

		SLV2Plugin p = g_ptr_array_index((GPtrArray*)list, i);

		const int cmp = strcmp(slv2_value_as_uri(slv2_plugin_get_uri(p)),
		                       slv2_value_as_uri(uri));

		if (cmp == 0)
			return p;
		else if (cmp > 0)
			upper = i - 1;
		else
			lower = i + 1;
	}

	return NULL;
}

SLV2Plugin
slv2_plugins_get_at(SLV2Plugins list, unsigned index)
{
	if (index > INT_MAX)
		return NULL;
	else
		return (SLV2Plugin)g_ptr_array_index((GPtrArray*)list, (int)index);
}

