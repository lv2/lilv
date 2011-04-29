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

#define _XOPEN_SOURCE 500

#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "lv2/lv2plug.in/ns/ext/event/event-helpers.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"

#include "lilv/lilv.h"

#include "lilv-config.h"

#ifdef LILV_JACK_SESSION
#include <jack/session.h>
#include <glib.h>

GMutex* exit_mutex;
GCond*  exit_cond;
#endif /* LILV_JACK_SESSION */

#define MIDI_BUFFER_SIZE 1024

enum PortType {
	CONTROL,
	AUDIO,
	EVENT
};

struct Port {
	const LilvPort*   lilv_port;
	enum PortType     type;
	jack_port_t*      jack_port; /**< For audio/MIDI ports, otherwise NULL */
	float             control;  /**< For control ports, otherwise 0.0f */
	LV2_Event_Buffer* ev_buffer; /**< For MIDI ports, otherwise NULL */
	bool              is_input;
};

/** This program's data */
struct JackHost {
	jack_client_t*    jack_client;   /**< Jack client */
	const LilvPlugin* plugin;        /**< Plugin "class" (actually just a few strings) */
	LilvInstance*     instance;      /**< Plugin "instance" (loaded shared lib) */
	uint32_t          num_ports;     /**< Size of the two following arrays: */
	struct Port*      ports;         /**< Port array of size num_ports */
	LilvNode*         input_class;   /**< Input port class (URI) */
	LilvNode*         output_class;  /**< Output port class (URI) */
	LilvNode*         control_class; /**< Control port class (URI) */
	LilvNode*         audio_class;   /**< Audio port class (URI) */
	LilvNode*         event_class;   /**< Event port class (URI) */
	LilvNode*         midi_class;    /**< MIDI event class (URI) */
	LilvNode*         optional;      /**< lv2:connectionOptional port property */
};

/** URI map feature, for event types (we use only MIDI) */
#define MIDI_EVENT_ID 1
uint32_t
uri_to_id(LV2_URI_Map_Callback_Data callback_data,
          const char*               map,
          const char*               uri)
{
	/* Note a non-trivial host needs to use an actual dictionary here */
	if (!strcmp(map, LV2_EVENT_URI) && !strcmp(uri, LILV_EVENT_CLASS_MIDI))
		return MIDI_EVENT_ID;
	else
		return 0;  /* Refuse to map ID */
}

#define NS_EXT "http://lv2plug.in/ns/ext/"

static LV2_URI_Map_Feature uri_map         = { NULL, &uri_to_id };
static const LV2_Feature   uri_map_feature = { NS_EXT "uri-map", &uri_map };

const LV2_Feature* features[2] = { &uri_map_feature, NULL };

/** Abort and exit on error */
static void
die(const char* msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

/** Creates a port and connects the plugin instance to its data location.
 *
 * For audio ports, creates a jack port and connects plugin port to buffer.
 *
 * For control ports, sets controls array to default value and connects plugin
 * port to that element.
 */
void
create_port(struct JackHost* host,
            uint32_t         port_index,
            float            default_value)
{
	struct Port* const port = &host->ports[port_index];

	port->lilv_port = lilv_plugin_get_port_by_index(host->plugin, port_index);
	port->jack_port = NULL;
	port->control   = 0.0f;
	port->ev_buffer = NULL;

	lilv_instance_connect_port(host->instance, port_index, NULL);

	/* Get the port symbol for console printing */
	const LilvNode* symbol     = lilv_port_get_symbol(host->plugin, port->lilv_port);
	const char*     symbol_str = lilv_node_as_string(symbol);

	enum JackPortFlags jack_flags = 0;
	if (lilv_port_is_a(host->plugin, port->lilv_port, host->input_class)) {
		jack_flags     = JackPortIsInput;
		port->is_input = true;
	} else if (lilv_port_is_a(host->plugin, port->lilv_port, host->output_class)) {
		jack_flags     = JackPortIsOutput;
		port->is_input = false;
	} else if (lilv_port_has_property(host->plugin, port->lilv_port, host->optional)) {
		lilv_instance_connect_port(host->instance, port_index, NULL);
	} else {
		die("Mandatory port has unknown type (neither input or output)");
	}

	/* Set control values */
	if (lilv_port_is_a(host->plugin, port->lilv_port, host->control_class)) {
		port->type    = CONTROL;
		port->control = isnan(default_value) ? 0.0 : default_value;
		printf("%s = %f\n", symbol_str, host->ports[port_index].control);
	} else if (lilv_port_is_a(host->plugin, port->lilv_port, host->audio_class)) {
		port->type = AUDIO;
	} else if (lilv_port_is_a(host->plugin, port->lilv_port, host->event_class)) {
		port->type = EVENT;
	}

	/* Connect the port based on its type */
	switch (port->type) {
	case CONTROL:
		lilv_instance_connect_port(host->instance, port_index, &port->control);
		break;
	case AUDIO:
		port->jack_port = jack_port_register(
			host->jack_client, symbol_str, JACK_DEFAULT_AUDIO_TYPE, jack_flags, 0);
		break;
	case EVENT:
		port->jack_port = jack_port_register(
			host->jack_client, symbol_str, JACK_DEFAULT_MIDI_TYPE, jack_flags, 0);
		port->ev_buffer = lv2_event_buffer_new(MIDI_BUFFER_SIZE, LV2_EVENT_AUDIO_STAMP);
		lilv_instance_connect_port(host->instance, port_index, port->ev_buffer);
		break;
	default:
		/* FIXME: check if port connection is optional and die if not */
		lilv_instance_connect_port(host->instance, port_index, NULL);
		fprintf(stderr, "WARNING: Unknown port type, port not connected.\n");
	}
}

/** Jack process callback. */
int
jack_process_cb(jack_nframes_t nframes, void* data)
{
	struct JackHost* const host = (struct JackHost*)data;

	/* Prepare port buffers */
	for (uint32_t p = 0; p < host->num_ports; ++p) {
		if (!host->ports[p].jack_port)
			continue;

		if (host->ports[p].type == AUDIO) {
			/* Connect plugin port directly to Jack port buffer. */
			lilv_instance_connect_port(
				host->instance, p,
				jack_port_get_buffer(host->ports[p].jack_port, nframes));

		} else if (host->ports[p].type == EVENT) {
			/* Clear Jack event port buffer. */
			lv2_event_buffer_reset(host->ports[p].ev_buffer,
			                       LV2_EVENT_AUDIO_STAMP,
			                       (uint8_t*)(host->ports[p].ev_buffer + 1));

			if (host->ports[p].is_input) {
				void* buf = jack_port_get_buffer(host->ports[p].jack_port,
				                                 nframes);

				LV2_Event_Iterator iter;
				lv2_event_begin(&iter, host->ports[p].ev_buffer);

				for (uint32_t i = 0; i < jack_midi_get_event_count(buf); ++i) {
					jack_midi_event_t ev;
					jack_midi_event_get(&ev, buf, i);
					lv2_event_write(&iter,
					                ev.time, 0,
					                MIDI_EVENT_ID, ev.size, ev.buffer);
				}
			}
		}
	}

	/* Run plugin for this cycle */
	lilv_instance_run(host->instance, nframes);

	/* Deliver MIDI output */
	for (uint32_t p = 0; p < host->num_ports; ++p) {
		if (host->ports[p].jack_port
		    && !host->ports[p].is_input
		    && host->ports[p].type == EVENT) {

			void* buf = jack_port_get_buffer(host->ports[p].jack_port,
			                                 nframes);

			jack_midi_clear_buffer(buf);

			LV2_Event_Iterator iter;
			lv2_event_begin(&iter, host->ports[p].ev_buffer);

			for (uint32_t i = 0; i < iter.buf->event_count; ++i) {
				uint8_t*   data;
				LV2_Event* ev = lv2_event_get(&iter, &data);
				jack_midi_event_write(buf, ev->frames, data, ev->size);
				lv2_event_increment(&iter);
			}
		}
	}

	return 0;
}

#ifdef LILV_JACK_SESSION
void
jack_session_cb(jack_session_event_t* event, void* arg)
{
	struct JackHost* host = (struct JackHost*)arg;

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "lv2_jack_host %s %s",
	         lilv_node_as_uri(lilv_plugin_get_uri(host->plugin)),
	         event->client_uuid);

	event->command_line = strdup(cmd);
	jack_session_reply(host->jack_client, event);

	switch (event->type) {
	case JackSessionSave:
		break;
	case JackSessionSaveAndQuit:
		g_mutex_lock(exit_mutex);
		g_cond_signal(exit_cond);
		g_mutex_unlock(exit_mutex);
		break;
	case JackSessionSaveTemplate:
		break;
	}

	jack_session_event_free(event);
}
#endif /* LILV_JACK_SESSION */

static void
signal_handler(int ignored)
{
#ifdef LILV_JACK_SESSION
	g_mutex_lock(exit_mutex);
	g_cond_signal(exit_cond);
	g_mutex_unlock(exit_mutex);
#endif
}

int
main(int argc, char** argv)
{
	struct JackHost host;
	host.jack_client = NULL;
	host.num_ports   = 0;
	host.ports       = NULL;

#ifdef LILV_JACK_SESSION
	if (!g_thread_supported()) {
		g_thread_init(NULL);
	}
	exit_mutex = g_mutex_new();
	exit_cond  = g_cond_new();
#endif

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* Find all installed plugins */
	LilvWorld* world = lilv_world_new();
	lilv_world_load_all(world);
	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);

	/* Set up the port classes this app supports */
	host.input_class   = lilv_new_uri(world, LILV_PORT_CLASS_INPUT);
	host.output_class  = lilv_new_uri(world, LILV_PORT_CLASS_OUTPUT);
	host.control_class = lilv_new_uri(world, LILV_PORT_CLASS_CONTROL);
	host.audio_class   = lilv_new_uri(world, LILV_PORT_CLASS_AUDIO);
	host.event_class   = lilv_new_uri(world, LILV_PORT_CLASS_EVENT);
	host.midi_class    = lilv_new_uri(world, LILV_EVENT_CLASS_MIDI);
	host.optional      = lilv_new_uri(world, LILV_NAMESPACE_LV2
	                                  "connectionOptional");

#ifdef LILV_JACK_SESSION
	if (argc != 2 && argc != 3) {
		fprintf(stderr, "Usage: %s PLUGIN_URI [JACK_UUID]\n", argv[0]);
#else
	if (argc != 2) {
		fprintf(stderr, "Usage: %s PLUGIN_URI\n", argv[0]);
#endif
		lilv_world_free(world);
		return EXIT_FAILURE;
	}

	const char* const plugin_uri_str = argv[1];

	printf("Plugin:    %s\n", plugin_uri_str);

	LilvNode* plugin_uri = lilv_new_uri(world, plugin_uri_str);
	host.plugin = lilv_plugins_get_by_uri(plugins, plugin_uri);
	lilv_node_free(plugin_uri);

	if (!host.plugin) {
		fprintf(stderr, "Failed to find plugin %s.\n", plugin_uri_str);
		lilv_world_free(world);
		return EXIT_FAILURE;
	}

	/* Get the plugin's name */
	LilvNode*   name     = lilv_plugin_get_name(host.plugin);
	const char* name_str = lilv_node_as_string(name);

	/* Truncate plugin name to suit JACK (if necessary) */
	char* jack_name = NULL;
	if (strlen(name_str) >= (unsigned)jack_client_name_size() - 1) {
		jack_name = calloc(jack_client_name_size(), sizeof(char));
		strncpy(jack_name, name_str, jack_client_name_size() - 1);
	} else {
		jack_name = strdup(name_str);
	}

	/* Connect to JACK */
	printf("JACK Name: %s\n\n", jack_name);
#ifdef LILV_JACK_SESSION
	const char* const jack_uuid_str = (argc > 2) ? argv[2] : NULL;
	if (jack_uuid_str) {
		host.jack_client = jack_client_open(jack_name, JackSessionID, NULL,
		                                    jack_uuid_str);
	}
#endif

	if (!host.jack_client) {
		host.jack_client = jack_client_open(jack_name, JackNullOption, NULL);
	}

	free(jack_name);
	lilv_node_free(name);

	if (!host.jack_client)
		die("Failed to connect to JACK.\n");

	/* Instantiate the plugin */
	host.instance = lilv_plugin_instantiate(
		host.plugin, jack_get_sample_rate(host.jack_client), features);
	if (!host.instance)
		die("Failed to instantiate plugin.\n");

	jack_set_process_callback(host.jack_client, &jack_process_cb, (void*)(&host));
#ifdef LILV_JACK_SESSION
	jack_set_session_callback(host.jack_client, &jack_session_cb, (void*)(&host));
#endif

	/* Create ports */
	host.num_ports = lilv_plugin_get_num_ports(host.plugin);
	host.ports     = calloc((size_t)host.num_ports, sizeof(struct Port));
	float* default_values = calloc(lilv_plugin_get_num_ports(host.plugin),
	                               sizeof(float));
	lilv_plugin_get_port_ranges_float(host.plugin, NULL, NULL, default_values);

	for (uint32_t i = 0; i < host.num_ports; ++i)
		create_port(&host, i, default_values[i]);

	free(default_values);

	/* Activate plugin and JACK */
	lilv_instance_activate(host.instance);
	jack_activate(host.jack_client);

	/* Run */
#ifdef LILV_JACK_SESSION
	printf("\nPress Ctrl-C to quit: ");
	fflush(stdout);
	g_cond_wait(exit_cond, exit_mutex);
#else
	printf("\nPress enter to quit: ");
	fflush(stdout);
	getc(stdin);
#endif
	printf("\n");

	/* Deactivate JACK */
	jack_deactivate(host.jack_client);

	for (uint32_t i = 0; i < host.num_ports; ++i) {
		if (host.ports[i].jack_port != NULL) {
			jack_port_unregister(host.jack_client, host.ports[i].jack_port);
			host.ports[i].jack_port = NULL;
		}
		if (host.ports[i].ev_buffer != NULL) {
			free(host.ports[i].ev_buffer);
		}
	}
	jack_client_close(host.jack_client);

	/* Deactivate plugin */
	lilv_instance_deactivate(host.instance);
	lilv_instance_free(host.instance);

	/* Clean up */
	free(host.ports);
	lilv_node_free(host.input_class);
	lilv_node_free(host.output_class);
	lilv_node_free(host.control_class);
	lilv_node_free(host.audio_class);
	lilv_node_free(host.event_class);
	lilv_node_free(host.midi_class);
	lilv_node_free(host.optional);
	lilv_world_free(world);

#ifdef LILV_JACK_SESSION
	g_mutex_free(exit_mutex);
	g_cond_free(exit_cond);
#endif

	return 0;
}
