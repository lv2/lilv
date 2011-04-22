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

#include "slv2_internal.h"

/** Ownership of value and label is taken */
SLV2ScalePoint
slv2_scale_point_new(SLV2Value value, SLV2Value label)
{
	SLV2ScalePoint point = (SLV2ScalePoint)malloc(sizeof(struct _SLV2ScalePoint));
	point->value = value;
	point->label = label;
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
