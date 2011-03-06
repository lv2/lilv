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
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "slv2_internal.h"

/** Ownership of value and label is taken */
SLV2ScalePoint
slv2_scale_point_new(SLV2Value value, SLV2Value label)
{
	SLV2ScalePoint point = (SLV2ScalePoint)malloc(sizeof(struct _SLV2ScalePoint));
	point->value = value;
	point->label= label;
	return point;
}

void
slv2_scale_point_free(SLV2ScalePoint point)
{
	slv2_value_free(point->value);
	slv2_value_free(point->label);
	free(point);
}

SLV2_API
SLV2Value
slv2_scale_point_get_value(SLV2ScalePoint p)
{
	return p->value;
}

SLV2_API
SLV2Value
slv2_scale_point_get_label(SLV2ScalePoint p)
{
	return p->label;
}

