// Copyright 2007-2019 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "lilv_internal.h"

#include <lilv/lilv.h>
#include <sord/sord.h>
#include <zix/tree.h>

#include <stdlib.h>
#include <string.h>

typedef enum {
  LILV_LANG_MATCH_NONE,    ///< Language does not match at all
  LILV_LANG_MATCH_PARTIAL, ///< Partial (language, but not country) match
  LILV_LANG_MATCH_EXACT    ///< Exact (language and country) match
} LilvLangMatch;

static LilvLangMatch
lilv_lang_matches(const char* a, const char* b)
{
  if (!a || !b) {
    return LILV_LANG_MATCH_NONE;
  }

  if (!strcmp(a, b)) {
    return LILV_LANG_MATCH_EXACT;
  }

  const char*  a_dash     = strchr(a, '-');
  const size_t a_lang_len = a_dash ? (size_t)(a_dash - a) : strlen(a);
  const char*  b_dash     = strchr(b, '-');
  const size_t b_lang_len = b_dash ? (size_t)(b_dash - b) : strlen(b);

  if (a_lang_len == b_lang_len && !strncmp(a, b, a_lang_len)) {
    return LILV_LANG_MATCH_PARTIAL;
  }

  return LILV_LANG_MATCH_NONE;
}

static LilvNodes*
lilv_nodes_from_matches_i18n(LilvWorld* const    world,
                             SordIter* const     stream,
                             const SordQuadIndex field)
{
  LilvNodes*      values  = lilv_nodes_new();
  const SordNode* nolang  = NULL; // Untranslated value
  const SordNode* partial = NULL; // Partial language match
  const char*     syslang = world->lang;
  FOREACH_MATCH (stream) {
    const SordNode* value = sord_iter_get_node(stream, field);
    if (sord_node_get_type(value) == SORD_LITERAL) {
      const char* lang = sord_node_get_language(value);

      if (!lang) {
        nolang = value;
      } else {
        switch (lilv_lang_matches(lang, syslang)) {
        case LILV_LANG_MATCH_EXACT:
          // Exact language match, add to results
          zix_tree_insert(
            (ZixTree*)values, lilv_node_new_from_node(world, value), NULL);
          break;
        case LILV_LANG_MATCH_PARTIAL:
          // Partial language match, save in case we find no exact
          partial = value;
          break;
        case LILV_LANG_MATCH_NONE:
          break;
        }
      }
    } else {
      zix_tree_insert(
        (ZixTree*)values, lilv_node_new_from_node(world, value), NULL);
    }
  }
  sord_iter_free(stream);

  if (lilv_nodes_size(values) > 0) {
    return values;
  }

  const SordNode* best = nolang;
  if ((syslang && partial) || !best) {
    best = partial;
  }

  if (best) {
    zix_tree_insert(
      (ZixTree*)values, lilv_node_new_from_node(world, best), NULL);
  } else {
    // No matches whatsoever
    lilv_nodes_free(values);
    values = NULL;
  }

  return values;
}

static LilvNodes*
lilv_nodes_from_matches_all(LilvWorld* const    world,
                            SordIter* const     stream,
                            const SordQuadIndex field)
{
  LilvNodes* const values = lilv_nodes_new();
  FOREACH_MATCH (stream) {
    const SordNode* value = sord_iter_get_node(stream, field);
    LilvNode*       node  = lilv_node_new_from_node(world, value);
    if (node) {
      zix_tree_insert((ZixTree*)values, node, NULL);
    }
  }
  sord_iter_free(stream);
  return values;
}

LilvNodes*
lilv_nodes_from_matches(LilvWorld* world, SordIter* stream, SordQuadIndex field)
{
  if (sord_iter_end(stream)) {
    sord_iter_free(stream);
    return NULL;
  }

  return world->opt.filter_language
           ? lilv_nodes_from_matches_i18n(world, stream, field)
           : lilv_nodes_from_matches_all(world, stream, field);
}
