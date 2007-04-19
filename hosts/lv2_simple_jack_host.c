/* SLV2 Simple Jack Host Example
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 *  
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <slv2/slv2.h>

int
main(/*int argc, char** argv*/)
{
	SLV2Model model = slv2_model_new();
	slv2_model_load_all(model);
	

	/*printf("********** All plugins **********\n");

	SLV2Plugins plugins = slv2_model_get_all_plugins(model);

	for (unsigned i=0; i < slv2_plugins_size(plugins); ++i) {
		SLV2Plugin p = slv2_plugins_get_at(plugins, i);
		printf("Plugin: %s\n", slv2_plugin_get_uri(p));
	}

	slv2_plugins_free(plugins);*/
	
	
	printf("********** Plugins with MIDI input **********\n");
	
	/*const char* query = 
    	"PREFIX : <http://lv2plug.in/ontology#>\n"
		//"PREFIX llext: <http://ll-plugins.nongnu.org/lv2/ext/>\n"
		"SELECT DISTINCT ?plugin WHERE {\n"
		"	?plugin a     :Plugin ;\n"
		"           :port ?port .\n"
		"   ?port  :symbol \"in\". \n"
		//"	        :port [ a llext:MidiPort; a :InputPort ] .\n"
		"}\n";

	SLV2Plugins plugins = slv2_model_get_plugins_by_query(model, query);

	for (unsigned i=0; i < slv2_plugins_size(plugins); ++i) {
		SLV2Plugin p = slv2_plugins_get_at(plugins, i);
		printf("Plugin: %s\n", slv2_plugin_get_uri(p));
	}
	
	slv2_plugins_free(plugins);
	*/

	slv2_model_free(model);
	
	return 0;
}
