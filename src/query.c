/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "lilv_internal.h"

LilvMatches
lilv_plugin_find_statements(const LilvPlugin* plugin,
                            LilvNode          subject,
                            LilvNode          predicate,
                            LilvNode          object)
{
	lilv_plugin_load_if_necessary(plugin);
	SordQuad pat = { subject, predicate, object, NULL };
	return sord_find(plugin->world->model, pat);
}

typedef enum {
	LILV_LANG_MATCH_NONE,    ///< Language does not match at all
	LILV_LANG_MATCH_PARTIAL, ///< Partial (language, but not country) match
	LILV_LANG_MATCH_EXACT    ///< Exact (language and country) match
} LilvLangMatch;

static LilvLangMatch
lilv_lang_matches(const char* a, const char* b)
{
	if (!strcmp(a, b)) {
		return LILV_LANG_MATCH_EXACT;
	}

	const char*  a_dash     = strchr(a, '-');
	const size_t a_lang_len = a_dash ? (a_dash - a) : 0;
	const char*  b_dash     = strchr(b, '-');
	const size_t b_lang_len = b_dash ? (b_dash - b) : 0;

	if (a_lang_len && b_lang_len) {
		if (a_lang_len == b_lang_len && !strncmp(a, b, a_lang_len)) {
			return LILV_LANG_MATCH_PARTIAL;  // e.g. a="en-gb", b="en-ca"
		}
	} else if (a_lang_len && !strncmp(a, b, a_lang_len)) {
		return LILV_LANG_MATCH_PARTIAL;  // e.g. a="en", b="en-ca"
	} else if (b_lang_len && !strncmp(a, b, b_lang_len)) {
		return LILV_LANG_MATCH_PARTIAL;  // e.g. a="en-ca", b="en"
	}
	return LILV_LANG_MATCH_NONE;
}

LilvValues*
lilv_values_from_stream_objects_i18n(const LilvPlugin* p,
                                     LilvMatches       stream)
{
	LilvValues* values  = lilv_values_new();
	LilvNode    nolang  = NULL;  // Untranslated value
	LilvNode    partial = NULL;  // Partial language match
	char*       syslang = lilv_get_lang();
	FOREACH_MATCH(stream) {
		LilvNode value = lilv_match_object(stream);
		if (sord_node_get_type(value) == SORD_LITERAL) {
			const char*   lang = sord_node_get_language(value);
			LilvLangMatch lm   = LILV_LANG_MATCH_NONE;
			if (lang) {
				lm = (syslang)
					? lilv_lang_matches(lang, syslang)
					: LILV_LANG_MATCH_PARTIAL;
			} else {
				nolang = value;
				if (!syslang) {
					lm = LILV_LANG_MATCH_EXACT;
				}
			}

			if (lm == LILV_LANG_MATCH_EXACT) {
				// Exact language match, add to results
				lilv_array_append(values, lilv_value_new_from_node(p->world, value));
			} else if (lm == LILV_LANG_MATCH_PARTIAL) {
				// Partial language match, save in case we find no exact
				partial = value;
			}
		} else {
			lilv_array_append(values, lilv_value_new_from_node(p->world, value));
		}
	}
	lilv_match_end(stream);
	free(syslang);

	if (lilv_values_size(values) > 0) {
		return values;
	}

	LilvNode best = nolang;
	if (syslang && partial) {
		// Partial language match for system language
		best = partial;
	} else if (!best) {
		// No languages matches at all, and no untranslated value
		// Use any value, if possible
		best = partial;
	}

	if (best) {
		lilv_array_append(values, lilv_value_new_from_node(p->world, best));
	} else {
		// No matches whatsoever
		lilv_values_free(values);
		values = NULL;
	}

	return values;
}

LilvValues*
lilv_values_from_stream_objects(const LilvPlugin* p,
                                LilvMatches       stream)
{
	if (lilv_matches_end(stream)) {
		lilv_match_end(stream);
		return NULL;
	} else if (p->world->opt.filter_language) {
		return lilv_values_from_stream_objects_i18n(p, stream);
	} else {
		LilvValues* values = lilv_values_new();
		FOREACH_MATCH(stream) {
			lilv_array_append(
				values,
				lilv_value_new_from_node(
					p->world,
					lilv_match_object(stream)));
		}
		lilv_match_end(stream);
		return values;
	}
}
