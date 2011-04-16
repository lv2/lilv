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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "slv2/slv2.h"

#include "slv2-config.h"

void
list_plugins(SLV2Plugins list, bool show_names)
{
	for (SLV2Iter i = slv2_plugins_begin(list);
	     !slv2_iter_end(i);
	     i = slv2_iter_next(i)) {
		SLV2Plugin p = slv2_plugins_get(list, i);
		if (show_names) {
			SLV2Value n = slv2_plugin_get_name(p);
			printf("%s\n", slv2_value_as_string(n));
			slv2_value_free(n);
		} else {
			printf("%s\n", slv2_value_as_uri(slv2_plugin_get_uri(p)));
		}
	}
}

void
print_version()
{
	printf("lv2_list (slv2) " SLV2_VERSION "\n");
	printf("Copyright 2007-2011 David Robillard <http://drobilla.net>\n");
	printf("License: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
}

void
print_usage()
{
	printf("Usage: lv2_list [OPTIONS]\n");
	printf("List all installed LV2 plugins.\n");
	printf("\n");
	printf("  -n, --names    Show names instead of URIs\n");
	printf("  --help         Display this help and exit\n");
	printf("  --version      Output version information and exit\n");
	printf("\n");
	printf("The environment variable LV2_PATH can be used to control where\n");
	printf("this (and all other slv2 based LV2 hosts) will search for plugins.\n");
}

int
main(int argc, char** argv)
{
	bool show_names = false;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--names") || !strcmp(argv[i], "-n")) {
			show_names = true;
		} else if (!strcmp(argv[i], "--version")) {
			print_version();
			return 0;
		} else if (!strcmp(argv[i], "--help")) {
			print_usage();
			return 0;
		} else {
			print_usage();
			return 1;
		}
	}

	SLV2World world = slv2_world_new();
	slv2_world_load_all(world);

	SLV2Plugins plugins = slv2_world_get_all_plugins(world);

	list_plugins(plugins, show_names);

	slv2_world_free(world);

	return 0;
}
