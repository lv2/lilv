/* SLV2
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 *  
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include <rasqal.h>
#include <slv2/slv2.h>

raptor_uri* slv2_ontology_uri = NULL;


void
slv2_init()
{
	rasqal_init();

	slv2_ontology_uri = raptor_new_uri((const unsigned char*)
		"file://" LV2_TTL_PATH);
}


void
slv2_finish()
{
	raptor_free_uri(slv2_ontology_uri);
	slv2_ontology_uri = NULL;
	
	rasqal_finish();
}

