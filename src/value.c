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
#include <assert.h>
#include <raptor.h>
#include <slv2/value.h>
#include "slv2_internal.h"


/* private */
SLV2Value
slv2_value_new(SLV2ValueType type, const char* str)
{
	SLV2Value val = (SLV2Value)malloc(sizeof(struct _SLV2Value));
	val->str_val = strdup(str);
	val->type = type;

	//printf("New value, t=%d, %s\n", type, str);

	if (type == SLV2_VALUE_INT) {
		char* endptr = 0;
		val->val.int_val = strtol(str, &endptr, 10);
	} else if (type == SLV2_VALUE_FLOAT) {
		char* endptr = 0;
		val->val.float_val = strtod(str, &endptr);
	} else {
		val->val.int_val = 0;
	}

	return val;
}


SLV2Value
slv2_value_duplicate(SLV2Value val)
{
	SLV2Value result = (SLV2Value)malloc(sizeof(struct _SLV2Value));
	result->str_val = strdup(val->str_val);
	result->type = val->type;
	result->val = val->val;
	return result;
}


void
slv2_value_free(SLV2Value val)
{
	if (val) {
		free(val->str_val);
		free(val);
	}
}


bool
slv2_value_equals(SLV2Value value, SLV2Value other)
{
	if (value->type != other->type)
		return false;
	else if (value && other)
		return ! strcmp(value->str_val, other->str_val);
	else
		return true;
}


char*
slv2_value_get_turtle_token(SLV2Value value)
{
	size_t len    = 0;
	char*  result = NULL;

	switch (value->type) {
	case SLV2_VALUE_GUI:
	case SLV2_VALUE_URI:
		len = strlen(value->str_val) + 3;
		result = calloc(len, sizeof(char));
		snprintf(result, len, "<%s>", value->str_val);
		break;
	case SLV2_VALUE_STRING:
		result = strdup(value->str_val);
		break;
	case SLV2_VALUE_INT:
	    // INT64_MAX is 9223372036854775807 (19 digits) + 1 for sign
		len = 20;
		result = calloc(len, sizeof(char));
		snprintf(result, len, "%d", value->val.int_val);
		break;
	case SLV2_VALUE_FLOAT:
		len = 20; // FIXME: proper maximum value?
		result = calloc(len, sizeof(char));
		snprintf(result, len, ".1%f", value->val.float_val);
		break;
	}
	
	return result;
}


bool
slv2_value_is_uri(SLV2Value value)
{
	return (value->type == SLV2_VALUE_URI || value->type == SLV2_VALUE_GUI);
}


const char*
slv2_value_as_uri(SLV2Value value)
{
	assert(slv2_value_is_uri(value) || slv2_value_is_gui(value));
	return value->str_val;
}


bool
slv2_value_is_literal(SLV2Value value)
{
	// No blank nodes
	return (value->type != SLV2_VALUE_URI);
}


bool
slv2_value_is_string(SLV2Value value)
{
	return (value->type == SLV2_VALUE_STRING);
}


const char*
slv2_value_as_string(SLV2Value value)
{
	return value->str_val;
}


bool
slv2_value_is_int(SLV2Value value)
{
	return (value->type == SLV2_VALUE_INT);
}


int
slv2_value_as_int(SLV2Value value)
{
	assert(slv2_value_is_int(value));
	return value->val.int_val;
}


bool
slv2_value_is_float(SLV2Value value)
{
	return (value->type == SLV2_VALUE_FLOAT);
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


bool
slv2_value_is_gui(SLV2Value value)
{
	return (value->type == SLV2_VALUE_GUI);
}


SLV2GUIType
slv2_value_as_gui_type(SLV2Value value)
{
	assert(slv2_value_is_gui(value));
	return value->val.gui_type_val;
}

