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
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "lilv_internal.h"

static void
lilv_value_set_numerics_from_string(LilvValue* val)
{
	char* locale;
	char* endptr;

	switch (val->type) {
	case LILV_VALUE_URI:
	case LILV_VALUE_BLANK:
	case LILV_VALUE_STRING:
		break;
	case LILV_VALUE_INT:
		// FIXME: locale kludge, need a locale independent strtol
		locale = lilv_strdup(setlocale(LC_NUMERIC, NULL));
		setlocale(LC_NUMERIC, "POSIX");
		val->val.int_val = strtol(val->str_val, &endptr, 10);
		setlocale(LC_NUMERIC, locale);
		free(locale);
		break;
	case LILV_VALUE_FLOAT:
		// FIXME: locale kludge, need a locale independent strtod
		locale = lilv_strdup(setlocale(LC_NUMERIC, NULL));
		setlocale(LC_NUMERIC, "POSIX");
		val->val.float_val = strtod(val->str_val, &endptr);
		setlocale(LC_NUMERIC, locale);
		free(locale);
		break;
	case LILV_VALUE_BOOL:
		val->val.bool_val = (!strcmp(val->str_val, "true"));
		break;
	}
}

/** Note that if @a type is numeric or boolean, the returned value is corrupt
 * until lilv_value_set_numerics_from_string is called.  It is not
 * automatically called from here to avoid overhead and imprecision when the
 * exact string value is known.
 */
LilvValue*
lilv_value_new(LilvWorld* world, LilvValueType type, const char* str)
{
	LilvValue* val = malloc(sizeof(struct LilvValueImpl));
	val->world = world;
	val->type  = type;

	switch (type) {
	case LILV_VALUE_URI:
		val->val.uri_val = sord_new_uri(world->world, (const uint8_t*)str);
		assert(val->val.uri_val);
		val->str_val = (char*)sord_node_get_string(val->val.uri_val);
		break;
	case LILV_VALUE_BLANK:
	case LILV_VALUE_STRING:
	case LILV_VALUE_INT:
	case LILV_VALUE_FLOAT:
	case LILV_VALUE_BOOL:
		val->str_val = lilv_strdup(str);
		break;
	}

	return val;
}

/** Create a new LilvValue from @a node, or return NULL if impossible */
LilvValue*
lilv_value_new_from_node(LilvWorld* world, const SordNode* node)
{
	LilvValue*    result       = NULL;
	SordNode*     datatype_uri = NULL;
	LilvValueType type         = LILV_VALUE_STRING;

	switch (sord_node_get_type(node)) {
	case SORD_URI:
		type                = LILV_VALUE_URI;
		result              = malloc(sizeof(struct LilvValueImpl));
		result->world       = world;
		result->type        = LILV_VALUE_URI;
		result->val.uri_val = lilv_node_copy(node);
		result->str_val     = (char*)sord_node_get_string(result->val.uri_val);
		break;
	case SORD_LITERAL:
		datatype_uri = sord_node_get_datatype(node);
		if (datatype_uri) {
			if (sord_node_equals(datatype_uri, world->xsd_boolean_node))
				type = LILV_VALUE_BOOL;
			else if (sord_node_equals(datatype_uri, world->xsd_decimal_node)
			         || sord_node_equals(datatype_uri, world->xsd_double_node))
				type = LILV_VALUE_FLOAT;
			else if (sord_node_equals(datatype_uri, world->xsd_integer_node))
				type = LILV_VALUE_INT;
			else
				LILV_ERRORF("Unknown datatype %s\n", sord_node_get_string(datatype_uri));
		}
		result = lilv_value_new(world, type, (const char*)sord_node_get_string(node));
		switch (result->type) {
		case LILV_VALUE_INT:
		case LILV_VALUE_FLOAT:
		case LILV_VALUE_BOOL:
			lilv_value_set_numerics_from_string(result);
		default:
			break;
		}
		break;
	case SORD_BLANK:
		type   = LILV_VALUE_BLANK;
		result = lilv_value_new(world, type, (const char*)sord_node_get_string(node));
		break;
	default:
		assert(false);
	}

	return result;
}

LILV_API
LilvValue*
lilv_new_uri(LilvWorld* world, const char* uri)
{
	return lilv_value_new(world, LILV_VALUE_URI, uri);
}

LILV_API
LilvValue*
lilv_new_string(LilvWorld* world, const char* str)
{
	return lilv_value_new(world, LILV_VALUE_STRING, str);
}

LILV_API
LilvValue*
lilv_new_int(LilvWorld* world, int val)
{
	char str[32];
	snprintf(str, sizeof(str), "%d", val);
	LilvValue* ret = lilv_value_new(world, LILV_VALUE_INT, str);
	ret->val.int_val = val;
	return ret;
}

LILV_API
LilvValue*
lilv_new_float(LilvWorld* world, float val)
{
	char str[32];
	snprintf(str, sizeof(str), "%f", val);
	LilvValue* ret = lilv_value_new(world, LILV_VALUE_FLOAT, str);
	ret->val.float_val = val;
	return ret;
}

LILV_API
LilvValue*
lilv_new_bool(LilvWorld* world, bool val)
{
	LilvValue* ret = lilv_value_new(world, LILV_VALUE_BOOL, val ? "true" : "false");
	ret->val.bool_val = val;
	return ret;
}

LILV_API
LilvValue*
lilv_value_duplicate(const LilvValue* val)
{
	if (val == NULL)
		return NULL;

	LilvValue* result = malloc(sizeof(struct LilvValueImpl));
	result->world = val->world;
	result->type = val->type;

	if (val->type == LILV_VALUE_URI) {
		result->val.uri_val = lilv_node_copy(val->val.uri_val);
		result->str_val = (char*)sord_node_get_string(result->val.uri_val);
	} else {
		result->str_val = lilv_strdup(val->str_val);
		result->val = val->val;
	}

	return result;
}

LILV_API
void
lilv_value_free(LilvValue* val)
{
	if (val) {
		if (val->type == LILV_VALUE_URI) {
			lilv_node_free(val->world, val->val.uri_val);
		} else {
			free(val->str_val);
		}
		free(val);
	}
}

LILV_API
bool
lilv_value_equals(const LilvValue* value, const LilvValue* other)
{
	if (value == NULL && other == NULL)
		return true;
	else if (value == NULL || other == NULL)
		return false;
	else if (value->type != other->type)
		return false;

	switch (value->type) {
	case LILV_VALUE_URI:
		return sord_node_equals(value->val.uri_val, other->val.uri_val);
	case LILV_VALUE_BLANK:
	case LILV_VALUE_STRING:
		return !strcmp(value->str_val, other->str_val);
	case LILV_VALUE_INT:
		return (value->val.int_val == other->val.int_val);
	case LILV_VALUE_FLOAT:
		return (value->val.float_val == other->val.float_val);
	case LILV_VALUE_BOOL:
		return (value->val.bool_val == other->val.bool_val);
	}

	return false; /* shouldn't get here */
}

LILV_API
char*
lilv_value_get_turtle_token(const LilvValue* value)
{
	size_t len    = 0;
	char*  result = NULL;
	char*  locale = NULL;

	switch (value->type) {
	case LILV_VALUE_URI:
		len = strlen(value->str_val) + 3;
		result = calloc(len, 1);
		snprintf(result, len, "<%s>", value->str_val);
		break;
	case LILV_VALUE_BLANK:
		len = strlen(value->str_val) + 3;
		result = calloc(len, 1);
		snprintf(result, len, "_:%s", value->str_val);
		break;
	case LILV_VALUE_STRING:
	case LILV_VALUE_BOOL:
		result = lilv_strdup(value->str_val);
		break;
	case LILV_VALUE_INT:
		// INT64_MAX is 9223372036854775807 (19 digits) + 1 for sign
		// FIXME: locale kludge, need a locale independent snprintf
		locale = lilv_strdup(setlocale(LC_NUMERIC, NULL));
		len = 20;
		result = calloc(len, 1);
		setlocale(LC_NUMERIC, "POSIX");
		snprintf(result, len, "%d", value->val.int_val);
		setlocale(LC_NUMERIC, locale);
		break;
	case LILV_VALUE_FLOAT:
		// FIXME: locale kludge, need a locale independent snprintf
		locale = lilv_strdup(setlocale(LC_NUMERIC, NULL));
		len = 20; // FIXME: proper maximum value?
		result = calloc(len, 1);
		setlocale(LC_NUMERIC, "POSIX");
		snprintf(result, len, "%f", value->val.float_val);
		setlocale(LC_NUMERIC, locale);
		break;
	}

	free(locale);

	return result;
}

LILV_API
bool
lilv_value_is_uri(const LilvValue* value)
{
	return (value && value->type == LILV_VALUE_URI);
}

LILV_API
const char*
lilv_value_as_uri(const LilvValue* value)
{
	assert(lilv_value_is_uri(value));
	return value->str_val;
}

LilvNode
lilv_value_as_node(const LilvValue* value)
{
	assert(lilv_value_is_uri(value));
	return value->val.uri_val;
}

LILV_API
bool
lilv_value_is_blank(const LilvValue* value)
{
	return (value && value->type == LILV_VALUE_BLANK);
}

LILV_API
const char*
lilv_value_as_blank(const LilvValue* value)
{
	assert(lilv_value_is_blank(value));
	return value->str_val;
}

LILV_API
bool
lilv_value_is_literal(const LilvValue* value)
{
	if (!value)
		return false;

	switch (value->type) {
	case LILV_VALUE_STRING:
	case LILV_VALUE_INT:
	case LILV_VALUE_FLOAT:
		return true;
	default:
		return false;
	}
}

LILV_API
bool
lilv_value_is_string(const LilvValue* value)
{
	return (value && value->type == LILV_VALUE_STRING);
}

LILV_API
const char*
lilv_value_as_string(const LilvValue* value)
{
	return value->str_val;
}

LILV_API
bool
lilv_value_is_int(const LilvValue* value)
{
	return (value && value->type == LILV_VALUE_INT);
}

LILV_API
int
lilv_value_as_int(const LilvValue* value)
{
	assert(value);
	assert(lilv_value_is_int(value));
	return value->val.int_val;
}

LILV_API
bool
lilv_value_is_float(const LilvValue* value)
{
	return (value && value->type == LILV_VALUE_FLOAT);
}

LILV_API
float
lilv_value_as_float(const LilvValue* value)
{
	assert(lilv_value_is_float(value) || lilv_value_is_int(value));
	if (lilv_value_is_float(value))
		return value->val.float_val;
	else // lilv_value_is_int(value)
		return (float)value->val.int_val;
}

LILV_API
bool
lilv_value_is_bool(const LilvValue* value)
{
	return (value && value->type == LILV_VALUE_BOOL);
}

LILV_API
bool
lilv_value_as_bool(const LilvValue* value)
{
	assert(value);
	assert(lilv_value_is_bool(value));
	return value->val.bool_val;
}
