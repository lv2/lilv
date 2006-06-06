/* LibSLV2 Jack Example Host
 * Copyright (C) 2006 Dave Robillard <drobilla@connect.carleton.ca>
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
#include <slv2/slv2.h>
#include <jack/jack.h>


/** This program's data */
struct JackHost {
	jack_client_t* jack_client; /**< Jack client */
	SLV2Plugin*    plugin;      /**< Plugin "class" (actually just a few strings) */
	SLV2Instance*  instance;    /**< Plugin "instance" (loaded shared lib) */
	unsigned long  num_ports;   /**< Size of the two following arrays: */
	jack_port_t**  jack_ports;  /**< For audio ports, otherwise NULL */
	float*         controls;    /**< For control ports, otherwise 0.0f */
};


void die(const char* msg);
void create_port(struct JackHost* host, unsigned long port_index);
int  jack_process_cb(jack_nframes_t nframes, void* data);
void list_plugins(SLV2List list);


int
main(int argc, char** argv)
{
	struct JackHost host;
	host.jack_client = NULL;
	host.num_ports   = 0;
	host.jack_ports  = NULL;
	host.controls    = NULL;
	
	/* Find all installed plugins */
	SLV2List plugins = slv2_list_new();
	slv2_list_load_all(plugins);
	//slv2_list_load_bundle(plugins, "http://www.scs.carleton.ca/~drobilla/files/Amp-swh.lv2");

	/* Find the plugin to run */
	const char* plugin_uri = (argc == 2) ? argv[1] : NULL;
	
	if (!plugin_uri) {
		fprintf(stderr, "\nYou must specify a plugin URI to load.\n");
		fprintf(stderr, "\nKnown plugins:\n\n");
		list_plugins(plugins);
		return EXIT_FAILURE;
	}

	printf("URI:\t%s\n", plugin_uri);
	host.plugin = slv2_list_get_plugin_by_uri(plugins, plugin_uri);
	
	if (!host.plugin) {
		fprintf(stderr, "Failed to find plugin %s.\n", plugin_uri);
		slv2_list_free(plugins);
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
	host.jack_ports = calloc(host.num_ports, sizeof(jack_port_t*));
	host.controls   = calloc(host.num_ports, sizeof(float*));
	
	for (unsigned long i=0; i < host.num_ports; ++i)
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
	slv2_list_free(plugins);
	
	printf("Shutting down JACK.\n");
	for (unsigned long i=0; i < host.num_ports; ++i) {
		if (host.jack_ports[i] != NULL) {
			jack_port_unregister(host.jack_client, host.jack_ports[i]);
			host.jack_ports[i] = NULL;
		}
	}
	jack_client_close(host.jack_client);
	
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
            unsigned long    port_index)
{
	/* Make sure this is a float port */
	enum SLV2DataType type = slv2_port_get_data_type(host->plugin, port_index);
	if (type != SLV2_DATA_TYPE_FLOAT)
		die("Unrecognized data type, aborting.");
	
	/* Get the port symbol (label) for console printing */
	char* symbol = slv2_port_get_symbol(host->plugin, port_index);
	
	/* Initialize the port array elements */
	host->jack_ports[port_index] = NULL;
	host->controls[port_index]   = 0.0f;
	
	/* Get the 'class' of the port (control input, audio output, etc) */
	enum SLV2PortClass class = slv2_port_get_class(host->plugin, port_index);
	
	/* Connect the port based on it's 'class' */
	switch (class) {
	case SLV2_CONTROL_RATE_INPUT:
		host->controls[port_index] = slv2_port_get_default_value(host->plugin, port_index);
		slv2_instance_connect_port(host->instance, port_index, &host->controls[port_index]);
		printf("Set %s to %f\n", symbol, host->controls[port_index]);
		break;
	case SLV2_CONTROL_RATE_OUTPUT:
		slv2_instance_connect_port(host->instance, port_index, &host->controls[port_index]);
		break;
	case SLV2_AUDIO_RATE_INPUT:
		host->jack_ports[port_index] = jack_port_register(host->jack_client,
			symbol, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
		break;
	case SLV2_AUDIO_RATE_OUTPUT:
		host->jack_ports[port_index] = jack_port_register(host->jack_client,
			symbol, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		break;
	default:
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
	for (unsigned long i=0; i < host->num_ports; ++i)
		if (host->jack_ports[i] != NULL)
			slv2_instance_connect_port(host->instance, i,
				jack_port_get_buffer(host->jack_ports[i], nframes));
	
	/* Run plugin for this cycle */
	slv2_instance_run(host->instance, nframes);

	return 0;
}


void
list_plugins(SLV2List list)
{
	for (int i=0; i < slv2_list_get_length(list); ++i) {
		const SLV2Plugin* const p = slv2_list_get_plugin_by_index(list, i);
		printf("%s\n", slv2_plugin_get_uri(p));
	}
}
