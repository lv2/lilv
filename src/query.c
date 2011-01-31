/* SLV2
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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
#include <redland.h>
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
	librdf_statement* q = librdf_new_statement_from_nodes(
		plugin->world->world,
		subject   ? slv2_node_copy(subject)   : NULL,
		predicate ? slv2_node_copy(predicate) : NULL,
		object    ? slv2_node_copy(object)    : NULL);
	librdf_stream* results = librdf_model_find_statements(plugin->rdf, q);
	librdf_free_statement(q);
	return results;
}


SLV2Values
slv2_values_from_stream_i18n(SLV2Plugin  p,
                             SLV2Matches stream)
{
	SLV2Values values = slv2_values_new();
	SLV2Node   nolang = NULL;
	FOREACH_MATCH(stream) {
		SLV2Node value = MATCH_OBJECT(stream);
		if (librdf_node_is_literal(value)) {
			const char* lang = librdf_node_get_literal_value_language(value);
			if (lang) {
				if (!strcmp(lang, slv2_get_lang())) {
					raptor_sequence_push(
						values, slv2_value_new_string(
							p->world, (const char*)librdf_node_get_literal_value(value)));
				}
			} else {
				nolang = value;
			}
		}
		break;
	}
	END_MATCH(stream);

	if (slv2_values_size(values) == 0) {
		// No value with a matching language, use untranslated default
		if (nolang) {
			raptor_sequence_push(
				values, slv2_value_new_string(
					p->world, (const char*)librdf_node_get_literal_value(nolang)));
		} else {
			slv2_values_free(values);
			values = NULL;
		}
	}

	return values;
}
