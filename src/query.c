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
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "slv2/types.h"
#include "slv2/collections.h"
#include "slv2/plugin.h"
#include "slv2/util.h"
#include "slv2_internal.h"

SLV2Matches
slv2_plugin_find_statements(SLV2Plugin plugin,
                            SLV2Node   subject,
                            SLV2Node   predicate,
                            SLV2Node   object)
{
	slv2_plugin_load_if_necessary(plugin);
	SordTuple pat = { subject, predicate, object, NULL };
	return sord_find(plugin->world->model, pat);
}

SLV2Values
slv2_values_from_stream_i18n(SLV2Plugin  p,
                             SLV2Matches stream)
{
	SLV2Values values = slv2_values_new();
	SLV2Node   nolang = NULL;
	FOREACH_MATCH(stream) {
		SLV2Node value = slv2_match_object(stream);
		if (sord_node_get_type(value) == SORD_LITERAL) {
			const char* lang = sord_literal_get_lang(value);
			if (lang) {
				if (!strcmp(lang, slv2_get_lang())) {
					g_ptr_array_add(
						values, (uint8_t*)slv2_value_new_string(
							p->world, (const char*)sord_node_get_string(value)));
				}
			} else {
				nolang = value;
			}
		}
		break;
	}
	slv2_match_end(stream);

	if (slv2_values_size(values) == 0) {
		// No value with a matching language, use untranslated default
		if (nolang) {
			g_ptr_array_add(
				values, (uint8_t*)slv2_value_new_string(
					p->world, (const char*)sord_node_get_string(nolang)));
		} else {
			slv2_values_free(values);
			values = NULL;
		}
	}

	return values;
}
