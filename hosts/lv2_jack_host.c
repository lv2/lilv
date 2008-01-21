/* jack_host - SLV2 Jack Host
 * Copyright (C) 2007 Dave Robillard <drobilla.net>
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

#include CONFIG_H_PATH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <slv2/slv2.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include "lv2-miditype.h"
#include "lv2-midifunctions.h"
#include "jack_compat.h"

#define MIDI_BUFFER_SIZE 1024

enum PortDirection {
	INPUT,
	OUTPUT
};

enum PortType {
	CONTROL,
	AUDIO,
	MIDI
};

struct Port {
	SLV2Port           slv2_port;
	enum PortDirection direction;
	enum PortType      type;
	jack_port_t*       jack_port;   /**< For audio and MIDI ports, otherwise NULL */
	float              control;     /**< For control ports, otherwise 0.0f */
	LV2_MIDI*          midi_buffer; /**< For midi ports, otherwise NULL */
};


/** This program's data */
struct JackHost {
	jack_client_t* jack_client;   /**< Jack client */
	SLV2Plugin     plugin;        /**< Plugin "class" (actually just a few strings) */
	SLV2Instance   instance;      /**< Plugin "instance" (loaded shared lib) */
	uint32_t       num_ports;     /**< Size of the two following arrays: */
	struct Port*   ports;         /** Port array of size num_ports */
	SLV2Value      input_class;   /**< Input port class (URI) */
	SLV2Value      output_class;  /**< Output port class (URI) */
	SLV2Value      control_class; /**< Control port class (URI) */
	SLV2Value      audio_class;   /**< Audio port class (URI) */
	SLV2Value      midi_class;    /**< MIDI port class (URI) */
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
	host.ports       = NULL;
	
	/* Find all installed plugins */
	SLV2World world = slv2_world_new();
	slv2_world_load_all(world);
	SLV2Plugins plugins = slv2_world_get_all_plugins(world);
	
	/* Set up the port classes this app supports */
	host.input_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_INPUT);
	host.output_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_OUTPUT);
	host.control_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_CONTROL);
	host.audio_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_AUDIO);
	host.midi_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_MIDI);

	/* Find the plugin to run */
	const char* plugin_uri = (argc == 2) ? argv[1] : NULL;
	
	if (!plugin_uri) {
		fprintf(stderr, "\nYou must specify a plugin URI to load.\n");
		fprintf(stderr, "\nKnown plugins:\n\n");
		list_plugins(plugins);
		slv2_world_free(world);
		return EXIT_FAILURE;
	}

	printf("URI:\t%s\n", plugin_uri);
	host.plugin = slv2_plugins_get_by_uri(plugins, plugin_uri);
	
	if (!host.plugin) {
		fprintf(stderr, "Failed to find plugin %s.\n", plugin_uri);
		slv2_world_free(world);
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
	host.num_ports = slv2_plugin_get_num_ports(host.plugin);
	host.ports = calloc((size_t)host.num_ports, sizeof(struct Port));
	
	for (uint32_t i=0; i < host.num_ports; ++i)
		create_port(&host, i);
	
	/* Activate plugin and JACK */
	slv2_instance_activate(host.instance);
	jack_activate(host.jack_client);
	
	/* Run */
	printf("Press enter to quit: ");
	getc(stdin);
	printf("\n");
	
	/* Deactivate JACK */
	jack_deactivate(host.jack_client);
	
	printf("Shutting down JACK.\n");
	for (unsigned long i=0; i < host.num_ports; ++i) {
		if (host.ports[i].jack_port != NULL) {
			jack_port_unregister(host.jack_client, host.ports[i].jack_port);
			host.ports[i].jack_port = NULL;
		}
		if (host.ports[i].midi_buffer != NULL) {
			lv2midi_free(host.ports[i].midi_buffer);
		}
	}
	jack_client_close(host.jack_client);
	
	/* Deactivate plugin */
	slv2_instance_deactivate(host.instance);
	slv2_instance_free(host.instance);
	
	/* Clean up */
	slv2_value_free(host.input_class);
	slv2_value_free(host.output_class);
	slv2_value_free(host.control_class);
	slv2_value_free(host.audio_class);
	slv2_value_free(host.midi_class);
	slv2_plugins_free(world, plugins);
	slv2_world_free(world);

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
            uint32_t         port_index)
{
	struct Port* const port = &host->ports[port_index];

	port->slv2_port   = slv2_plugin_get_port_by_index(host->plugin, port_index);
	port->jack_port   = NULL;
	port->control     = 0.0f;
	port->midi_buffer = NULL;

	slv2_instance_connect_port(host->instance, port_index, NULL);

	/* Get the port symbol (label) for console printing */
	char* symbol = slv2_port_get_symbol(host->plugin, port->slv2_port);

	enum JackPortFlags jack_flags = 0;
	if (slv2_port_is_a(host->plugin, port->slv2_port, host->input_class)) {
		jack_flags = JackPortIsInput;
		port->direction = INPUT;
	} else if (slv2_port_is_a(host->plugin, port->slv2_port, host->output_class)) {
		jack_flags = JackPortIsOutput;
		port->direction = OUTPUT;
	} else if (slv2_port_has_property(host->plugin, port->slv2_port, SLV2_NAMESPACE_LV2 "connectionOptional")) {
		slv2_instance_connect_port(host->instance, port_index, NULL);
	} else {
		die("Mandatory port has unknown type (neither input or output)");
	}
	
	/* Set control values */
	if (slv2_port_is_a(host->plugin, port->slv2_port, host->control_class)) {
		port->type = CONTROL;
		port->control = slv2_port_get_default_value(host->plugin, port->slv2_port);
		printf("Set %s to %f\n", symbol, host->ports[port_index].control);
	} else if (slv2_port_is_a(host->plugin, port->slv2_port, host->audio_class)) {
		port->type = AUDIO;
	} else if (slv2_port_is_a(host->plugin, port->slv2_port, host->midi_class)) {
		port->type = MIDI;
	}

	/* Connect the port based on it's type */
	switch (port->type) {
		case CONTROL:
			slv2_instance_connect_port(host->instance, port_index, &port->control);
			break;
		case AUDIO:
			port->jack_port = jack_port_register(host->jack_client,
					symbol, JACK_DEFAULT_AUDIO_TYPE, jack_flags, 0);
			break;
		case MIDI:
			port->jack_port = jack_port_register(host->jack_client,
					symbol, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
			port->midi_buffer = lv2midi_new(MIDI_BUFFER_SIZE);
			slv2_instance_connect_port(host->instance, port_index, port->midi_buffer);
			break;
		default:
			// FIXME: check if port connection is is optional and die if not
			slv2_instance_connect_port(host->instance, port_index, NULL);
			fprintf(stderr, "WARNING: Unknown port type, port not connected.\n");
	}

	free(symbol);
}


/** Jack process callback. */
int
jack_process_cb(jack_nframes_t nframes, void* data)
{
	struct JackHost* const host = (struct JackHost*)data;

	/* Connect inputs */
	for (uint32_t p=0; p < host->num_ports; ++p) {
		if (!host->ports[p].jack_port)
			continue;

		if (host->ports[p].type == AUDIO) {

			slv2_instance_connect_port(host->instance, p,
					jack_port_get_buffer(host->ports[p].jack_port, nframes));

		} else if (host->ports[p].type == MIDI) {

			lv2midi_reset_buffer(host->ports[p].midi_buffer);

			if (host->ports[p].direction == INPUT) {
				void* jack_buffer = jack_port_get_buffer(host->ports[p].jack_port, nframes);

				lv2midi_reset_buffer(host->ports[p].midi_buffer);

				LV2_MIDIState state;
				lv2midi_reset_state(&state, host->ports[p].midi_buffer, nframes);

				const jack_nframes_t event_count
					= jack_midi_get_event_count(jack_buffer);

				jack_midi_event_t ev;

				for (jack_nframes_t e=0; e < event_count; ++e) {
					jack_midi_event_get(&ev, jack_buffer, e);
					lv2midi_put_event(&state, (double)ev.time, ev.size, ev.buffer);
				}

			}
		}
	}
	

	/* Run plugin for this cycle */
	slv2_instance_run(host->instance, nframes);


	/* Deliver output */
	for (uint32_t p=0; p < host->num_ports; ++p) {
		if (host->ports[p].jack_port
				&& host->ports[p].direction == INPUT
				&& host->ports[p].type == MIDI) {

			void* jack_buffer = jack_port_get_buffer(host->ports[p].jack_port, nframes);

			jack_midi_clear_buffer(jack_buffer);

			LV2_MIDIState state;
			lv2midi_reset_state(&state, host->ports[p].midi_buffer, nframes);
			
			double         timestamp = 0.0f;
			uint32_t       size      = 0;
			unsigned char* data      = NULL;

			const uint32_t event_count = state.midi->event_count;

			for (uint32_t i=0; i < event_count; ++i) {
				lv2midi_get_event(&state, &timestamp, &size, &data);

#if defined(JACK_MIDI_NEEDS_NFRAMES)
				jack_midi_event_write(jack_buffer,
                              (jack_nframes_t)timestamp, data, size, nframes);
#else
				jack_midi_event_write(jack_buffer,
						(jack_nframes_t)timestamp, data, size);
#endif

				lv2midi_increment(&state);
			}

		}
	}

	return 0;
}


void
list_plugins(SLV2Plugins list)
{
	for (unsigned i=0; i < slv2_plugins_size(list); ++i) {
		SLV2Plugin p = slv2_plugins_get_at(list, i);
		printf("%s\n", slv2_plugin_get_uri(p));
	}
}
