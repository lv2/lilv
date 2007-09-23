/* SLV2
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * Author: Lars Luthman
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

#ifndef __SLV2_PLUGINUIINSTANCE_H__
#define __SLV2_PLUGINUIINSTANCE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <slv2/lv2_gui.h>
#include <slv2/plugin.h>

/** \defgroup lib Plugin UI library access
 *
 * An SLV2UIInstance is an instantiated UI for a SLV2Plugin. UI instances
 * are loaded from dynamically loaded libraries.  These functions interact 
 * with the UI code in the binary library only, they do not read data files
 * in any way.
 * 
 * @{
 */


typedef struct _SLV2UIInstanceImpl* SLV2UIInstanceImpl;

  
struct _GtkWidget;


/** Instance of a plugin UI.
 *
 * All details are in hidden in the pimpl member to avoid making the
 * implementation a part of the ABI.
 */
typedef struct _SLV2UIInstance {
	SLV2UIInstanceImpl pimpl; ///< Private implementation
}* SLV2UIInstance;



/** Instantiate a plugin UI.
 *
 * The returned object represents shared library objects loaded into memory,
 * it must be cleaned up with slv2_ui_instance_free when no longer
 * needed.
 * 
 * \a plugin is not modified or directly referenced by the returned object
 * (instances store only a copy of the plugin's URI).
 * 
 * \a host_features NULL-terminated array of features the host supports.
 * NULL may be passed if the host supports no additional features (unlike
 * the LV2 specification - SLV2 takes care of it).
 *
 * \return NULL if instantiation failed.
 */
SLV2UIInstance
slv2_plugin_ui_instantiate(SLV2Plugin                     plugin,
                           SLV2PluginUI                   gui,
                           LV2UI_Write_Function           write_function,
                           LV2UI_Command_Function         command_function,
                           LV2UI_Program_Change_Function  program_function,
                           LV2UI_Program_Save_Function    save_function,
                           LV2UI_Controller               controller,
                           const LV2_Host_Feature* const* host_features);


/** Free a plugin UI instance.
 *
 * \a instance is invalid after this call.
 */
void
slv2_ui_instance_free(SLV2UIInstance instance);


/** Get the widget for the UI instance.
 */
LV2UI_Widget
slv2_ui_instance_get_widget(SLV2UIInstance instance);


/** Get the LV2UI_Descriptor of the plugin UI instance.
 *
 * Normally hosts should not need to access the LV2UI_Descriptor directly,
 * use the slv2_ui_instance_* functions.
 *
 * The returned descriptor is shared and must not be deleted.
 */
const LV2UI_Descriptor*
slv2_ui_instance_get_descriptor(SLV2UIInstance instance);


/** Get the LV2UI_Handle of the plugin UI instance.
 *
 * Normally hosts should not need to access the LV2UI_Handle directly,
 * use the slv2_ui_instance_* functions.
 * 
 * The returned handle is shared and must not be deleted.
 */
LV2_Handle
slv2_ui_instance_get_handle(SLV2UIInstance instance);


/** @} */

#ifdef __cplusplus
}
#endif


#endif /* __SLV2_PLUGINUIINSTANCE_H__ */

