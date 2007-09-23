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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <librdf.h>
#include <slv2/types.h>
#include <slv2/pluginuis.h>
#include <slv2/pluginui.h>
#include <slv2/util.h>
#include "slv2_internal.h"

	
SLV2PluginUIs
slv2_plugin_uis_new()
{
	return raptor_new_sequence((void (*)(void*))&slv2_plugin_ui_free, NULL);
	//return raptor_new_sequence(NULL, NULL);
}


void
slv2_plugin_uis_free(SLV2PluginUIs list)
{
	raptor_free_sequence(list);
}


unsigned
slv2_plugin_uis_size(SLV2PluginUIs list)
{
	return raptor_sequence_size(list);
}


SLV2PluginUI
slv2_plugin_uis_get_by_uri(SLV2PluginUIs list, const char* uri)
{
	// good old fashioned binary search
	
	int lower = 0;
	int upper = raptor_sequence_size(list) - 1;
	int i;
	
	while (upper >= lower) {
		i = lower + ((upper - lower) / 2);

		SLV2PluginUI ui = raptor_sequence_get_at(list, i);

		int cmp = strcmp(slv2_plugin_ui_get_uri(ui), uri);

		if (cmp == 0)
			return ui;
		else if (cmp > 0)
			upper = i - 1;
		else
			lower = i + 1;
	}

	return NULL;
}


SLV2PluginUI
slv2_plugin_uis_get_at(SLV2PluginUIs list, unsigned index)
{	
	if (index > INT_MAX)
		return NULL;
	else
		return (SLV2PluginUI)raptor_sequence_get_at(list, (int)index);
}

