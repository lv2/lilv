/* ladspa2lv2
 * Copyright (C) 2007 Dave Robillard <drobilla@connect.carleton.ca>
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
#include <raptor.h>
#include <ladspa.h>
#include <dlfcn.h>

#define U(x) ((const unsigned char*)(x))


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


void
write_lv2_turtle(LADSPA_Descriptor* descriptor, const char* uri, const char* filename)
{
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
}


void
print_usage()
{
	printf("Usage: ladspa2slv2 /path/to/laddspalib.so index lv2_uri\n\n");
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
