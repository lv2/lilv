/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGE.
*/

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "slv2_internal.h"

SLV2Matches
slv2_plugin_find_statements(SLV2Plugin plugin,
                            SLV2Node   subject,
                            SLV2Node   predicate,
                            SLV2Node   object)
{
	slv2_plugin_load_if_necessary(plugin);
	SordQuad pat = { subject, predicate, object, NULL };
	return sord_find(plugin->world->model, pat);
}

typedef enum {
	SLV2_LANG_MATCH_NONE,    ///< Language does not match at all
	SLV2_LANG_MATCH_PARTIAL, ///< Partial (language, but not country) match
	SLV2_LANG_MATCH_EXACT    ///< Exact (language and country) match
} SLV2LangMatch;

static SLV2LangMatch
slv2_lang_matches(const char* a, const char* b)
{
	if (!strcmp(a, b)) {
		return SLV2_LANG_MATCH_EXACT;
	}

	const char*  a_dash     = strchr(a, '-');
	const size_t a_lang_len = a_dash ? (a_dash - a) : 0;
	const char*  b_dash     = strchr(b, '-');
	const size_t b_lang_len = b_dash ? (b_dash - b) : 0;

	if (a_lang_len && b_lang_len) {
		if (a_lang_len == b_lang_len && !strncmp(a, b, a_lang_len)) {
			return SLV2_LANG_MATCH_PARTIAL;  // e.g. a="en-gb", b="en-ca"
		}
	} else if (a_lang_len && !strncmp(a, b, a_lang_len)) {
		return SLV2_LANG_MATCH_PARTIAL;  // e.g. a="en", b="en-ca"
	} else if (b_lang_len && !strncmp(a, b, b_lang_len)) {
		return SLV2_LANG_MATCH_PARTIAL;  // e.g. a="en-ca", b="en"
	}
	return SLV2_LANG_MATCH_NONE;
}

SLV2Values
slv2_values_from_stream_objects_i18n(SLV2Plugin  p,
                                     SLV2Matches stream)
{
	SLV2Values values  = slv2_values_new();
	SLV2Node   nolang  = NULL;  // Untranslated value
	SLV2Node   partial = NULL;  // Partial language match
	char*      syslang = slv2_get_lang();
	FOREACH_MATCH(stream) {
		SLV2Node value = slv2_match_object(stream);
		if (sord_node_get_type(value) == SORD_LITERAL) {
			const char*   lang = sord_node_get_language(value);
			SLV2LangMatch lm   = SLV2_LANG_MATCH_NONE;
			if (lang) {
				lm = (syslang)
					? slv2_lang_matches(lang, syslang)
					: SLV2_LANG_MATCH_PARTIAL;
			} else {
				nolang = value;
				if (!syslang) {
					lm = SLV2_LANG_MATCH_EXACT;
				}
			}
			
			if (lm == SLV2_LANG_MATCH_EXACT) {
				// Exact language match, add to results
				slv2_array_append(values, slv2_value_new_from_node(p->world, value));
			} else if (lm == SLV2_LANG_MATCH_PARTIAL) {
				// Partial language match, save in case we find no exact
				partial = value;
			}
		} else {
			slv2_array_append(values, slv2_value_new_from_node(p->world, value));
		}
	}
	slv2_match_end(stream);
	free(syslang);

	if (slv2_values_size(values) > 0) {
		return values;
	}

	SLV2Node best = nolang;
	if (syslang && partial) {
		// Partial language match for system language
		best = partial;
	} else if (!best) {
		// No languages matches at all, and no untranslated value
		// Use any value, if possible
		best = partial;
	}

	if (best) {
		slv2_array_append(values, slv2_value_new_from_node(p->world, best));
	} else {
		// No matches whatsoever
		slv2_values_free(values);
		values = NULL;
	}

	return values;
}

SLV2Values
slv2_values_from_stream_objects(SLV2Plugin  p,
                                SLV2Matches stream)
{
	if (slv2_matches_end(stream)) {
		slv2_match_end(stream);
		return NULL;
	} else if (p->world->filter_language) {
		return slv2_values_from_stream_objects_i18n(p, stream);
	} else {
		SLV2Values values = slv2_values_new();
		FOREACH_MATCH(stream) {
			slv2_array_append(
				values,
				slv2_value_new_from_node(
					p->world,
					slv2_match_object(stream)));
		}
		slv2_match_end(stream);
		return values;
	}
}
