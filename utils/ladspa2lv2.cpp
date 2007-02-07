/* ladspa2lv2
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
#include <ladspa.h>
#include <dlfcn.h>
#include "raul/RDFWriter.h"

#define U(x) ((const unsigned char*)(x))

using Raul::RDFWriter;
using Raul::RdfId;
using Raul::Atom;

LADSPA_Descriptor*
load_ladspa_plugin(const char* lib_path, unsigned long index)
{
	void* const handle = dlopen(lib_path, RTLD_LAZY);
	if (handle == NULL)
		return NULL;

	LADSPA_Descriptor_Function df
		= (LADSPA_Descriptor_Function)dlsym(handle, "ladspa_descriptor");

	if (df == NULL) {
		dlclose(handle);
		return NULL;
	}	

	LADSPA_Descriptor* const descriptor = (LADSPA_Descriptor*)df(index);

	return descriptor;
}

#if 0
void
write_resource(raptor_serializer* serializer,
               const char* subject_uri,
               const char* predicate_uri,
               const char* object_uri)
{
	raptor_statement triple;

	triple.subject = (void*)raptor_new_uri(U(subject_uri));
	triple.subject_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;

	triple.predicate = (void*)raptor_new_uri(U(predicate_uri));
	triple.predicate_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;

	//if (object.type() == RdfId::RESOURCE) {
	triple.object = (void*)raptor_new_uri(U(object_uri));
	triple.object_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
	/*} else {
	  assert(object.type() == RdfId::ANONYMOUS);
	  triple.object = (unsigned char*)(strdup(object.to_string().c_str()));
	  triple.object_type = RAPTOR_IDENTIFIER_TYPE_ANONYMOUS;
	  }*/
	
	raptor_serialize_statement(serializer, &triple);
}
#endif

void
write_lv2_turtle(LADSPA_Descriptor* descriptor, const char* uri, const char* filename)
{
#if 0
	raptor_init();
	raptor_serializer* serializer = raptor_new_serializer("turtle");

	// Set up namespaces
	raptor_serialize_set_namespace(serializer, raptor_new_uri(
			U("http://www.w3.org/1999/02/22-rdf-syntax-ns#")), U("rdf"));
	raptor_serialize_set_namespace(serializer, raptor_new_uri(
			U("http://www.w3.org/2000/01/rdf-schema#")), U("rdfs"));
	raptor_serialize_set_namespace(serializer, raptor_new_uri(
			U("http://www.w3.org/2001/XMLSchema")), U("xsd"));
	raptor_serialize_set_namespace(serializer, raptor_new_uri(
			U("http://usefulinc.com/ns/doap#")), U("doap"));
	raptor_serialize_set_namespace(serializer, raptor_new_uri(
			U("http://xmlns.com/foaf/0.1/")), U("foaf"));
	raptor_serialize_set_namespace(serializer, raptor_new_uri(
			U("http://lv2plug.in/ontology#")), U("lv2"));
	
	raptor_serialize_start_to_filename(serializer, filename);


	// Write out plugin data
	
	write_resource(serializer, uri, 
		"http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "lv2:Plugin");
	
	write_resource(serializer, uri, 
		"http://usefulinc.com/ns/doap#name", descriptor->Name);

	
	
	raptor_serialize_end(serializer);
	raptor_free_serializer(serializer);
	raptor_finish();
#endif
	RDFWriter writer;

	writer.add_prefix("lv2", "http://lv2plug.in/ontology#");
	writer.add_prefix("doap", "http://usefulinc.com/ns/doap#");

	writer.start_to_filename(filename);

	RdfId plugin_id = RdfId(RdfId::RESOURCE, uri);

	writer.write(plugin_id,
		RdfId(RdfId::RESOURCE, "rdf:type"),
		RdfId(RdfId::RESOURCE, "lv2:Plugin"));
	
	writer.write(plugin_id,
		RdfId(RdfId::RESOURCE, "doap:name"),
		Atom(descriptor->Name));

	if (LADSPA_IS_HARD_RT_CAPABLE(descriptor->Properties))
		writer.write(plugin_id,
			RdfId(RdfId::RESOURCE, "lv2:property"),
		 	RdfId(RdfId::RESOURCE, "lv2:hardRTCapable"));
	
	for (uint32_t i=0; i < descriptor->PortCount; ++i) {
		char index_str[32];
		snprintf(index_str, 32, "%u", i);

		RdfId port_id(RdfId::ANONYMOUS, index_str);

		writer.write(plugin_id,
			RdfId(RdfId::RESOURCE, "lv2:port"),
			port_id);

		writer.write(port_id,
			RdfId(RdfId::RESOURCE, "lv2:index"),
		 	Atom((int32_t)i));
		
		writer.write(port_id,
			RdfId(RdfId::RESOURCE, "lv2:dataType"),
			RdfId(RdfId::RESOURCE, "lv2:float"));
		
		writer.write(port_id,
			RdfId(RdfId::RESOURCE, "lv2:name"),
		 	Atom(descriptor->PortNames[i]));
	}

	writer.finish();
}


void
print_usage()
{
	printf("Usage: ladspa2slv2 /path/to/laddspalib.so ladspa_index lv2_uri\n");
	printf("Partially convert a LADSPA plugin to an LV2 plugin.");
	printf("(This is a utility for developers, it will not generate a usable\n");
	printf("LV2 plugin directly).\n\n");
}


int
main(int argc, char** argv)
{
	if (argc != 4) {
		print_usage();
		return 1;
	}

	const char* const   lib_path = argv[1];
	const unsigned long index    = atol(argv[2]);
	const char* const   uri      = argv[3];


	LADSPA_Descriptor* descriptor = load_ladspa_plugin(lib_path, index);

	if (descriptor) {
		printf("Loaded %s : %lu\n", lib_path, index);
		write_lv2_turtle(descriptor, uri, "ladspaplugin.ttl");
	} else {
		printf("Failed to load %s : %lu\n", lib_path, index);
	}

	return 0;
}
