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
#include <jack/jack.h>


/** This program's data */
struct JackHost {
	jack_client_t* jack_client; /**< Jack client */
	SLV2Plugin*    plugin;      /**< Plugin "class" (actually just a few strings) */
	SLV2Instance*  instance;    /**< Plugin "instance" (loaded shared lib) */
	uint32_t       num_ports;   /**< Size of the two following arrays: */
	jack_port_t**  jack_ports;  /**< For audio ports, otherwise NULL */
	float*         controls;    /**< For control ports, otherwise 0.0f */
};


void die(const char* msg);
void create_port(struct JackHost* host, uint32_t port_index);
int  jack_process_cb(jack_nframes_t nframes, void* data);
void list_plugins(SLV2Plugins list);


int
main(int argc, char** argv)
{
	struct JackHost host;
	host.jack_client = NULL;
	host.num_ports   = 0;
	host.jack_ports  = NULL;
	host.controls    = NULL;
	
	slv2_init();

	/* Find all installed plugins */
	SLV2Plugins plugins = slv2_plugins_new();
	slv2_plugins_load_all(plugins);
	//slv2_plugins_load_bundle(plugins, "http://www.scs.carleton.ca/~drobilla/files/Amp-swh.lv2");

	/* Find the plugin to run */
	const char* plugin_uri = (argc == 2) ? argv[1] : NULL;
	
	if (!plugin_uri) {
		fprintf(stderr, "\nYou must specify a plugin URI to load.\n");
		fprintf(stderr, "\nKnown plugins:\n\n");
		list_plugins(plugins);
		return EXIT_FAILURE;
	}

	printf("URI:\t%s\n", plugin_uri);
	host.plugin = slv2_plugins_get_by_uri(plugins, plugin_uri);
	
	if (!host.plugin) {
		fprintf(stderr, "Failed to find plugin %s.\n", plugin_uri);
		slv2_plugins_free(plugins);
		return EXIT_FAILURE;
	}

	/* Get the plugin's name */
	char* name = slv2_plugin_get_name(host.plugin);
	printf("Name:\t%s\n", name);
	
	/* Connect to JACK (with plugin name as client name) */
	host.jack_client = jack_client_open(name, JackNullOption, NULL);
	free(name);
	if (!host.jack_client)
		die("Failed to connect to JACK.");
	else
		printf("Connected to JACK.\n");
	
	/* Instantiate the plugin */
	host.instance = slv2_plugin_instantiate(
		host.plugin, jack_get_sample_rate(host.jack_client), NULL);
	if (!host.instance)
		die("Failed to instantiate plugin.\n");
	else
		printf("Succesfully instantiated plugin.\n");

	jack_set_process_callback(host.jack_client, &jack_process_cb, (void*)(&host));
	
	/* Create ports */
	host.num_ports  = slv2_plugin_get_num_ports(host.plugin);
	host.jack_ports = calloc((size_t)host.num_ports, sizeof(jack_port_t*));
	host.controls   = calloc((size_t)host.num_ports, sizeof(float*));
	
	for (uint32_t i=0; i < host.num_ports; ++i)
		create_port(&host, i);
	
	/* Activate plugin and JACK */
	slv2_instance_activate(host.instance);
	jack_activate(host.jack_client);
	
	/* Run */
	printf("Press enter to quit: ");
	getc(stdin);
	printf("\n");
	
	/* Deactivate plugin and JACK */
	slv2_instance_free(host.instance);
	slv2_plugins_free(plugins);
	
	printf("Shutting down JACK.\n");
	for (unsigned long i=0; i < host.num_ports; ++i) {
		if (host.jack_ports[i] != NULL) {
			jack_port_unregister(host.jack_client, host.jack_ports[i]);
			host.jack_ports[i] = NULL;
		}
	}
	jack_client_close(host.jack_client);
	
	slv2_finish();

	return 0;
}


/** Abort and exit on error */
void
die(const char* msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}


/** Creates a port and connects the plugin instance to it's data location.
 *
 * For audio ports, creates a jack port and connects plugin port to buffer.
 *
 * For control ports, sets controls array to default value and connects plugin
 * port to that element.
 */
void
create_port(struct JackHost* host,
            uint32_t         index)
{
	SLV2PortID id = slv2_port_by_index(index);

	/* Get the port symbol (label) for console printing */
	char* symbol = slv2_port_get_symbol(host->plugin, id);
	
	/* Initialize the port array elements */
	host->jack_ports[index] = NULL;
	host->controls[index]   = 0.0f;
	
	/* Get the 'class' of the port (control input, audio output, etc) */
	SLV2PortClass class = slv2_port_get_class(host->plugin, id);
	
	/* Connect the port based on it's 'class' */
	switch (class) {
	case SLV2_CONTROL_INPUT:
		host->controls[index] = slv2_port_get_default_value(host->plugin, id);
		slv2_instance_connect_port(host->instance, index, &host->controls[index]);
		printf("Set %s to %f\n", symbol, host->controls[index]);
		break;
	case SLV2_CONTROL_OUTPUT:
		slv2_instance_connect_port(host->instance, index, &host->controls[index]);
		break;
	case SLV2_AUDIO_INPUT:
		host->jack_ports[index] = jack_port_register(host->jack_client,
			symbol, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
		break;
	case SLV2_AUDIO_OUTPUT:
		host->jack_ports[index] = jack_port_register(host->jack_client,
			symbol, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		break;
	default:
		// Simple examples don't have to be robust :)
		die("ERROR: Unknown port type, aborting messily!");
	}

	free(symbol);
}


/** Jack process callback. */
int
jack_process_cb(jack_nframes_t nframes, void* data)
{
	struct JackHost* host = (struct JackHost*)data;

	/* Connect plugin ports directly to JACK buffers */
	for (uint32_t i=0; i < host->num_ports; ++i)
		if (host->jack_ports[i] != NULL)
			slv2_instance_connect_port(host->instance, i,
				jack_port_get_buffer(host->jack_ports[i], nframes));
	
	/* Run plugin for this cycle */
	slv2_instance_run(host->instance, nframes);

	return 0;
}


void
list_plugins(SLV2Plugins list)
{
	for (unsigned i=0; i < slv2_plugins_size(list); ++i) {
		const SLV2Plugin* const p = slv2_plugins_get_at(list, i);
		printf("%s\n", slv2_plugin_get_uri(p));
	}
}
