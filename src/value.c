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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <locale.h>
#include <raptor.h>
#include "slv2/types.h"
#include "slv2/value.h"
#include "slv2_internal.h"


/* private */
static void
slv2_value_set_numerics_from_string(SLV2Value val)
{
	char* locale;
	char* endptr;

	switch (val->type) {
	case SLV2_VALUE_URI:
	case SLV2_VALUE_BLANK:
	case SLV2_VALUE_QNAME_UNUSED:
	case SLV2_VALUE_STRING:
		break;
	case SLV2_VALUE_INT:
		// FIXME: locale kludge, need a locale independent strtol
		locale = strdup(setlocale(LC_NUMERIC, NULL));
		setlocale(LC_NUMERIC, "POSIX");
		val->val.int_val = strtol(val->str_val, &endptr, 10);
		setlocale(LC_NUMERIC, locale);
		free(locale);
		break;
	case SLV2_VALUE_FLOAT:
		// FIXME: locale kludge, need a locale independent strtod
		locale = strdup(setlocale(LC_NUMERIC, NULL));
		setlocale(LC_NUMERIC, "POSIX");
		val->val.float_val = strtod(val->str_val, &endptr);
		setlocale(LC_NUMERIC, locale);
		free(locale);
		break;
	}
}


/* private
 * Note that if type is numeric, slv2_value_set_numerics_from_string MUST be
 * called or the returned value will be corrupt!  It is not automatically
 * called from here to avoid the parsing overhead and imprecision when the
 * true numeric value is already known.
 */
SLV2Value
slv2_value_new(SLV2World world, SLV2ValueType type, const char* str)
{
	SLV2Value val = (SLV2Value)malloc(sizeof(struct _SLV2Value));
	val->type = type;

	switch (type) {
	case SLV2_VALUE_URI:
		val->val.uri_val = librdf_new_node_from_uri_string(
			world->world, (const uint8_t*)str);
		assert(val->val.uri_val);
		val->str_val = (char*)librdf_uri_as_string(
			librdf_node_get_uri(val->val.uri_val));
		break;
	case SLV2_VALUE_QNAME_UNUSED:
	case SLV2_VALUE_BLANK:
	case SLV2_VALUE_STRING:
	case SLV2_VALUE_INT:
	case SLV2_VALUE_FLOAT:
		val->str_val = strdup(str);
		break;
	}

	return val;
}


/** Create a new SLV2Value from @a node, or return NULL if impossible */
SLV2Value
slv2_value_new_from_node(SLV2World world, librdf_node* node)
{
	SLV2Value     result       = NULL;
	librdf_uri*   datatype_uri = NULL;
	SLV2ValueType type         = SLV2_VALUE_STRING;

	switch (librdf_node_get_type(node)) {
	case LIBRDF_NODE_TYPE_RESOURCE:
		type                = SLV2_VALUE_URI;
		result              = (SLV2Value)malloc(sizeof(struct _SLV2Value));
		result->type        = SLV2_VALUE_URI;
		result->val.uri_val = slv2_node_copy(node);
		result->str_val     = (char*)librdf_uri_as_string(
			librdf_node_get_uri(result->val.uri_val));
		break;
	case LIBRDF_NODE_TYPE_LITERAL:
		datatype_uri = librdf_node_get_literal_value_datatype_uri(node);
		if (datatype_uri) {
			if (librdf_uri_equals(datatype_uri, librdf_node_get_uri(world->xsd_integer_node)))
				type = SLV2_VALUE_INT;
			else if (librdf_uri_equals(datatype_uri, librdf_node_get_uri(world->xsd_decimal_node)))
				type = SLV2_VALUE_FLOAT;
			else
				SLV2_ERRORF("Unknown datatype %s\n", librdf_uri_as_string(datatype_uri));
		}
		result = slv2_value_new(world, type, (const char*)librdf_node_get_literal_value(node));
		switch (result->type) {
		case SLV2_VALUE_URI:
		case SLV2_VALUE_BLANK:
		case SLV2_VALUE_STRING:
		case SLV2_VALUE_QNAME_UNUSED:
			break;
		case SLV2_VALUE_INT:
		case SLV2_VALUE_FLOAT:
			slv2_value_set_numerics_from_string(result);
		}
		break;
	case LIBRDF_NODE_TYPE_BLANK:
		type = SLV2_VALUE_BLANK;
		result = slv2_value_new(world, type, (const char*)librdf_node_get_blank_identifier(node));
		break;
	case LIBRDF_NODE_TYPE_UNKNOWN:
	default:
		SLV2_ERRORF("Unknown RDF node type %d\n", librdf_node_get_type(node));
		break;
	}

	return result;
}


SLV2Value
slv2_value_new_uri(SLV2World world, const char* uri)
{
	return slv2_value_new(world, SLV2_VALUE_URI, uri);
}


SLV2Value
slv2_value_new_string(SLV2World world, const char* str)
{
	return slv2_value_new(world, SLV2_VALUE_STRING, str);
}


SLV2Value
slv2_value_new_int(SLV2World world, int val)
{
	char str[32];
	snprintf(str, sizeof(str), "%d", val);
	SLV2Value ret = slv2_value_new(world, SLV2_VALUE_INT, str);
	ret->val.int_val = val;
	return ret;
}


SLV2Value
slv2_value_new_float(SLV2World world, float val)
{
	char str[32];
	snprintf(str, sizeof(str), "%f", val);
	SLV2Value ret = slv2_value_new(world, SLV2_VALUE_FLOAT, str);
	ret->val.float_val = val;
	return ret;
}


SLV2Value
slv2_value_duplicate(SLV2Value val)
{
	if (val == NULL)
		return val;

	SLV2Value result = (SLV2Value)malloc(sizeof(struct _SLV2Value));
	result->type = val->type;

	if (val->type == SLV2_VALUE_URI) {
		result->val.uri_val = slv2_node_copy(val->val.uri_val);
		result->str_val = (char*)librdf_uri_as_string(
			librdf_node_get_uri(val->val.uri_val));
	} else {
		result->str_val = strdup(val->str_val);
		result->val = val->val;
	}

	return result;
}


void
slv2_value_free(SLV2Value val)
{
	if (val) {
		if (val->type == SLV2_VALUE_URI)
			slv2_node_free(val->val.uri_val);
		else
			free(val->str_val);

		free(val);
	}
}


bool
slv2_value_equals(SLV2Value value, SLV2Value other)
{
	if (value == NULL && other == NULL)
		return true;
	else if (value == NULL || other == NULL)
		return false;
	else if (value->type != other->type)
		return false;

	switch (value->type) {
	case SLV2_VALUE_URI:
		return (librdf_node_equals(value->val.uri_val, other->val.uri_val) != 0);
	case SLV2_VALUE_BLANK:
	case SLV2_VALUE_STRING:
	case SLV2_VALUE_QNAME_UNUSED:
		return ! strcmp(value->str_val, other->str_val);
	case SLV2_VALUE_INT:
		return (value->val.int_val == other->val.int_val);
	case SLV2_VALUE_FLOAT:
		return (value->val.float_val == other->val.float_val);
	}

	return false; /* shouldn't get here */
}


char*
slv2_value_get_turtle_token(SLV2Value value)
{
	size_t len    = 0;
	char*  result = NULL;
	char*  locale = NULL;

	switch (value->type) {
	case SLV2_VALUE_URI:
		len = strlen(value->str_val) + 3;
		result = calloc(len, 1);
		snprintf(result, len, "<%s>", value->str_val);
		break;
	case SLV2_VALUE_BLANK:
		len = strlen(value->str_val) + 3;
		result = calloc(len, 1);
		snprintf(result, len, "_:%s", value->str_val);
		break;
	case SLV2_VALUE_STRING:
	case SLV2_VALUE_QNAME_UNUSED:
		result = strdup(value->str_val);
		break;
	case SLV2_VALUE_INT:
		// INT64_MAX is 9223372036854775807 (19 digits) + 1 for sign
		// FIXME: locale kludge, need a locale independent snprintf
		locale = strdup(setlocale(LC_NUMERIC, NULL));
		len = 20;
		result = calloc(len, 1);
		setlocale(LC_NUMERIC, "POSIX");
		snprintf(result, len, "%d", value->val.int_val);
		setlocale(LC_NUMERIC, locale);
		break;
	case SLV2_VALUE_FLOAT:
		// FIXME: locale kludge, need a locale independent snprintf
		locale = strdup(setlocale(LC_NUMERIC, NULL));
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


bool
slv2_value_is_uri(SLV2Value value)
{
	return (value && value->type == SLV2_VALUE_URI);
}


const char*
slv2_value_as_uri(SLV2Value value)
{
	assert(slv2_value_is_uri(value));
	return value->str_val;
}


/* private */
librdf_uri*
slv2_value_as_librdf_uri(SLV2Value value)
{
	assert(slv2_value_is_uri(value));
	return librdf_node_get_uri(value->val.uri_val);
}


bool
slv2_value_is_blank(SLV2Value value)
{
	return (value && value->type == SLV2_VALUE_BLANK);
}


const char*
slv2_value_as_blank(SLV2Value value)
{
	assert(slv2_value_is_blank(value));
	return value->str_val;
}


bool
slv2_value_is_literal(SLV2Value value)
{
	// No blank nodes
	return (value && value->type != SLV2_VALUE_URI);
}


bool
slv2_value_is_string(SLV2Value value)
{
	return (value && value->type == SLV2_VALUE_STRING);
}


const char*
slv2_value_as_string(SLV2Value value)
{
	return value->str_val;
}


bool
slv2_value_is_int(SLV2Value value)
{
	return (value && value->type == SLV2_VALUE_INT);
}


int
slv2_value_as_int(SLV2Value value)
{
	assert(value);
	assert(slv2_value_is_int(value));
	return value->val.int_val;
}


bool
slv2_value_is_float(SLV2Value value)
{
	return (value && value->type == SLV2_VALUE_FLOAT);
}


float
slv2_value_as_float(SLV2Value value)
{
	assert(slv2_value_is_float(value) || slv2_value_is_int(value));
	if (slv2_value_is_float(value))
		return value->val.float_val;
	else // slv2_value_is_int(value)
		return (float)value->val.int_val;
}

