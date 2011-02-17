/* SLV2
 * Copyright (C) 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef SLV2_SLV2_UI_H__
#define SLV2_SLV2_UI_H__

#include "slv2/slv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup slv2
 * @{
 */

typedef struct _SLV2UI* SLV2UI;   /**< Plugin UI. */
typedef void*           SLV2UIs;  /**< set<UI>. */

SLV2_COLLECTION(SLV2UIs, SLV2UI, slv2_uis)

/** Get a UI from @a uis by URI.
 * Return value is shared (stored in @a uis) and must not be freed or
 * modified by the caller in any way.
 * @return NULL if no UI with @a uri is found in @a list.
 */
SLV2_API
SLV2UI
slv2_uis_get_by_uri(SLV2UIs   uis,
                    SLV2Value uri);

/** Get a list of all UIs available for this plugin.
 * Note this returns the URI of the UI, and not the path/URI to its shared
 * library, use slv2_ui_get_library_uri with the values returned
 * here for that.
 *
 * Returned value must be freed by caller using slv2_uis_free.
 */
SLV2_API
SLV2UIs
slv2_plugin_get_uis(SLV2Plugin plugin);

/** @name Plugin UI
 * @{
 */

/** Get the URI of a Plugin UI.
 * @param ui The Plugin UI
 * @return a shared value which must not be modified or freed.
 */
SLV2_API
SLV2Value
slv2_ui_get_uri(SLV2UI ui);

/** Get the types (URIs of RDF classes) of a Plugin UI.
 * @param ui The Plugin UI
 * @return a shared value which must not be modified or freed.
 */
SLV2_API
SLV2Values
slv2_ui_get_classes(SLV2UI ui);

/** Check whether a plugin UI is a given type.
 * @param ui        The Plugin UI
 * @param class_uri The URI of the LV2 UI type to check this UI against
 */
SLV2_API
bool
slv2_ui_is_a(SLV2UI ui, SLV2Value class_uri);

/** Get the URI for a Plugin UI's bundle.
 * @param ui The Plugin UI
 * @return a shared value which must not be modified or freed.
 */
SLV2_API
SLV2Value
slv2_ui_get_bundle_uri(SLV2UI ui);

/** Get the URI for a Plugin UI's shared library.
 * @param ui The Plugin UI
 * @return a shared value which must not be modified or freed.
 */
SLV2_API
SLV2Value
slv2_ui_get_binary_uri(SLV2UI ui);

/** @} */
/** @name Plugin UI Instance
 * @{
 */

typedef struct _SLV2UIInstance* SLV2UIInstance;

/** Instantiate a plugin UI.
 * The returned object represents shared library objects loaded into memory,
 * it must be cleaned up with slv2_ui_instance_free when no longer
 * needed.
 *
 * @a plugin is not modified or directly referenced by the returned object
 * (instances store only a copy of the plugin's URI).
 *
 * @a host_features NULL-terminated array of features the host supports.
 * NULL may be passed if the host supports no additional features (unlike
 * the LV2 specification - SLV2 takes care of it).
 *
 * @return NULL if instantiation failed.
 */
SLV2_API
SLV2UIInstance
slv2_ui_instantiate(SLV2Plugin                plugin,
                    SLV2UI                    ui,
                    LV2UI_Write_Function      write_function,
                    LV2UI_Controller          controller,
                    const LV2_Feature* const* features);

/** Free a plugin UI instance.
 * @a instance is invalid after this call.
 * It is the caller's responsibility to ensure all references to the UI
 * instance (including any returned widgets) are cut before calling
 * this function.
 */
SLV2_API
void
slv2_ui_instance_free(SLV2UIInstance instance);

/** Get the widget for the UI instance.
 */
SLV2_API
LV2UI_Widget
slv2_ui_instance_get_widget(SLV2UIInstance instance);

/** Get the LV2UI_Descriptor of the plugin UI instance.
 * Normally hosts should not need to access the LV2UI_Descriptor directly,
 * use the slv2_ui_instance_* functions.
 *
 * The returned descriptor is shared and must not be deleted.
 */
SLV2_API
const LV2UI_Descriptor*
slv2_ui_instance_get_descriptor(SLV2UIInstance instance);

/** Get the LV2UI_Handle of the plugin UI instance.
 * Normally hosts should not need to access the LV2UI_Handle directly,
 * use the slv2_ui_instance_* functions.
 *
 * The returned handle is shared and must not be deleted.
 */
SLV2_API
LV2UI_Handle
slv2_ui_instance_get_handle(SLV2UIInstance instance);

/** @} */
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SLV2_SLV2_UI_H__ */
