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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slv2_internal.h"

SLV2Plugins
slv2_plugins_new()
{
	return g_sequence_new(NULL);
}

SLV2_API
void
slv2_plugins_free(SLV2World world, SLV2Plugins list)
{
	if (list && list != world->plugins)
		g_sequence_free(list);
}

SLV2_API
unsigned
slv2_plugins_size(SLV2Plugins list)
{
	return (list ? g_sequence_get_length((GSequence*)list) : 0); \
}

SLV2_API
SLV2Plugin
slv2_plugins_get_by_uri(SLV2Plugins list, SLV2Value uri)
{
	return (SLV2Plugin)slv2_sequence_get_by_uri(list, uri);
}

SLV2_API
SLV2Plugin
slv2_plugins_get_at(SLV2Plugins list, unsigned index)
{
	if (!list || index >= slv2_plugins_size(list)) {
		return NULL;
	} else {
		GSequenceIter* i = g_sequence_get_iter_at_pos((GSequence*)list, (int)index);
		return (SLV2Plugin)g_sequence_get(i);
	}
}

