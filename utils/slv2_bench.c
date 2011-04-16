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

#include <stdio.h>

#include "slv2/slv2.h"

#include "slv2-config.h"

void
print_version()
{
	printf("slv2_bench (slv2) " SLV2_VERSION "\n");
	printf("Copyright 2011-2011 David Robillard <http://drobilla.net>\n");
	printf("License: Simplified BSD License.\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
}

void
print_usage()
{
	printf("Usage: slv2_bench\n");
	printf("Load all discovered LV2 plugins.\n");
}

int
main(int argc, char** argv)
{
	SLV2World world = slv2_world_new();
	slv2_world_load_all(world);

	SLV2Plugins plugins = slv2_world_get_all_plugins(world);
	SLV2_FOREACH(p, plugins) {
		SLV2Plugin plugin = slv2_collection_get(plugins, p);
		slv2_plugin_get_class(plugin);
	}

	slv2_world_free(world);

	return 0;
}
