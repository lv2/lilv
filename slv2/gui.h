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

#ifndef __SLV2_GUI_H__
#define __SLV2_GUI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <slv2/types.h>
#include <slv2/port.h>
#include <slv2/values.h>

/** \defgroup gui Plugin GUI helpers
 *
 * SLV2 provides access to plugin GUIs, but does not depend on any widget sets
 * itself.  Paths to libraries and opaque pointers to objects are returned, but
 * the GUI extension itself (the URI of which can be had with slv2_gui_get_uri)
 * defines the specifics of how to use these objects/files/etc.
 *
 * @{
 */

/** Return the URI of the GUI type (ontology) this GUI corresponds to.
 * 
 * Returned string is shared and must not be freed by the caller.
 *
 * Time = O(1)
 */
const char*
slv2_gui_type_get_uri(SLV2GUIType type)
{
	// Only one for now...
	assert(type == SLV2_GTK2_GUI);
	return "http://ll-plugins.nongnu.org/lv2/ext/gtk2gui";
}


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_GUI_H__ */

