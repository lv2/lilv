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

#ifndef __SLV2_PLUGIN_UI_H__
#define __SLV2_PLUGIN_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup ui Plugin user interfaces
 *
 * @{
 */


/** Get the URI of a Plugin UI.
 *
 * \param ui The Plugin UI
 *
 * Time = O(1)
 */
const char*
slv2_plugin_ui_get_uri(SLV2PluginUI ui);


/** Get the URI of the a Plugin UI.
 *
 * \param ui The Plugin UI
 *
 * Time = O(1)
 */
SLV2Values
slv2_plugin_ui_get_types(SLV2PluginUI ui);


/** Check whether a plugin UI is a given type.
 *
 * \param ui The Plugin UI
 *
 * Time = O(1)
 */
bool
slv2_plugin_ui_is_type(SLV2PluginUI ui, const char* type_uri);
	

/** Get the URI for a Plugin UI's bundle.
 *
 * \param ui The Plugin UI
 *
 * Time = O(1)
 */
const char*
slv2_plugin_ui_get_bundle_uri(SLV2PluginUI ui);


/** Get the URI for a Plugin UI's shared library.
 *
 * \param ui The Plugin UI
 *
 * Time = O(1)
 */
const char*
slv2_plugin_ui_get_binary_uri(SLV2PluginUI ui);


#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PLUGIN_UI_H__ */

