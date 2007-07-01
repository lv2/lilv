/************************************************************************
 *
 * GTK2 in-process UI extension for LV2
 *
 * Copyright (C) 2006 Lars Luthman <lars.luthman@gmail.com>
 * 
 * Based on lv2.h, which is 
 *
 * Copyright (C) 2000-2002 Richard W.E. Furse, Paul Barton-Davis, Stefan
 * Westerfeld
 * Copyright (C) 2006 Steve Harris, Dave Robillard.
 *
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 *
 * This header is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 * USA.
 *
 ***********************************************************************/

/** @file
    This extension defines an interface that can be used in LV2 plugins and
    hosts to create GTK2 GUIs for plugins. The GUIs will be plugins that
    will reside in shared object files in a LV2 bundle and be referenced in 
    the RDF file using the triples
<pre>    
    <http://my.plugin> <http://ll-plugins.nongnu.org/lv2/ext/gtk2gui#gui> <http://my.plugingui>
    <http://my.plugingui> <http://ll-plugins.nongnu.org/lv2/ext/gtk2gui#binary> <mygui.so>
</pre>
    where <http://my.plugin> is the URI of the plugin, <http://my.plugingui> is
    the URI of the plugin GUI and <mygui.so> is the relative URI to the shared 
    object file. While it is possible to have the plugin GUI and the plugin in 
    the same shared object file it is probably a good idea to keep them 
    separate so that hosts that don't want GUIs don't have to load the GUI code.
    
    It's entirely possible to have multiple GUIs for the same plugin, or to have
    the GUI for a plugin in a different bundle from the actual plugin - this
    way people other than the plugin author can write plugin GUIs independently
    without editing the original plugin bundle. If a GUI is in a separate bundle
    the first triple above should be in that bundle's manifest.ttl file so that
    hosts can find the GUI when scanning the manifests.
    
    Note that the process that loads the shared object file containing the GUI
    code and the process that loads the shared object file containing the 
    actual plugin implementation does not have to be the same. There are many
    valid reasons for having the plugin and the GUI in different processes, or
    even on different machines. This means that you can _not_ use singletons
    and global variables and expect them to refer to the same objects in the
    GUI and the actual plugin. The function callback interface defined in this
    header is all you can expect to work.
    
    Since the LV2 specification itself allows for extensions that may add 
    new types of data and configuration parameters that plugin authors may 
    want to control with a GUI, this extension allows for meta-extensions that
    can extend the interface between the GUI and the host. See the
    instantiate() and extension_data() callback pointers for more details.
    
    Note that this extension is NOT a Host Feature. There is no way for a plugin
    to know whether the host that loads it supports GUIs or not, and the plugin
    must ALWAYS work without the GUI (although it may be rather useless unless
    it has been configured using the GUI in a previous session).
    
    GUIs written to this specification do not need to be threadsafe - the 
    functions defined below may only be called in the same thread as the GTK
    main loop is running in.
*/

#ifndef LV2_GTK2GUI_H
#define LV2_GTK2GUI_H

#include "lv2.h"


#ifdef __cplusplus
extern "C" {
#endif

/* The original header includes <gtk/gtkwidget.h>, but we don't want
   libslv2 to depend on GTK+ so we forward declare the _GtkWidget struct
   instead. */
struct _GtkWidget;



/** This handle indicates a particular instance of a GUI.
    It is valid to compare this to NULL (0 for C++) but otherwise the 
    host MUST not attempt to interpret it. The GUI plugin may use it to 
    reference internal instance data. */
typedef void* LV2UI_Handle;


/** This handle indicates a particular plugin instance, provided by the host.
    It is valid to compare this to NULL (0 for C++) but otherwise the 
    GUI plugin MUST not attempt to interpret it. The host may use it to 
    reference internal instance data. */
typedef void* LV2UI_Controller;


/** This is the type of the host-provided function that changes the value
    of a control rate float input port in a plugin instance. A function
    pointer of this type will be provided to the GUI bu the host in the
    instantiate() function. */
typedef void (*LV2UI_Set_Control_Function)(LV2UI_Controller controller,
                                           uint32_t         port,
                                           float            value);


typedef struct _LV2UI_Descriptor {
  
  /** The URI for this GUI (not for the plugin it controls). */
  const char* URI;
  
  /** Create a new GUI object and return a handle to it. This function works
      similarly to the instantiate() member in LV2_Descriptor, with the 
      additions that the URI for the plugin that this GUI is for is passed
      as a parameter, a function pointer and a controller handle are passed to
      allow the plugin to change control port values in the plugin 
      (control_function and controller) and a pointer to a GtkWidget pointer
      is passed, which the GUI plugin should set to point to a newly created
      widget which will be the main GUI for the plugin.
      
      The features array works like the one in the instantiate() member
      in LV2_Descriptor, except that the URIs should be denoted with the triples
      <pre>
      <http://my.plugingui> <http://ll-plugins.nongnu.org/lv2/dev/gtk2gui#optionalFeature> <http://my.guifeature>
      </pre>
      or 
      <pre>
      <http://my.plugingui> <http://ll-plugins.nongnu.org/lv2/dev/gtk2gui#requiredFeature> <http://my.guifeature>
      </pre>
      in the RDF file, instead of the lv2:optionalFeature or lv2:requiredFeature
      that is used by host features. These features are associated with the GUI,
      not with the plugin - they are not actually LV2 Host Features, they just
      use the same data structure.
      
      The same rules apply for these features as for normal host features -
      if a feature is listed as required in the RDF file and the host does not
      support it, it must not load the GUI.
  */
  LV2UI_Handle (*instantiate)(const struct _LV2UI_Descriptor* descriptor,
                              const char*                     plugin_uri,
                              const char*                     bundle_path,
                              LV2UI_Set_Control_Function      control_function,
                              LV2UI_Controller                controller,
                              struct _GtkWidget**             widget,
                              const LV2_Host_Feature**        features);

  
  /** Destroy the GUI object and the associated widget. 
   */
  void (*cleanup)(LV2UI_Handle  gui);
  
  /** Tell the GUI that a control port value has changed. This member may be
      set to NULL if the GUI is not interested in control port changes. 
  */
  void (*set_control)(LV2UI_Handle   gui,
                      uint32_t       port,
                      float          value);
  
  /** Returns a data structure associated with an extension URI, for example
      a struct containing additional function pointers. Avoid returning
      function pointers directly since standard C++ has no valid way of
      casting a void* to a function pointer. This member may be set to NULL
      if the GUI is not interested in supporting any extensions. This is similar
      to the extension_data() member in LV2_Descriptor.
  */
  void* (*extension_data)(LV2UI_Handle    gui,
                          const char*     uri);

} LV2UI_Descriptor;



/** Accessing a plugin GUI.
    
    A plugin programmer must include a function called "lv2ui_descriptor"
    with the following function prototype within the shared object
    file. This function will have C-style linkage (if you are using
    C++ this is taken care of by the 'extern "C"' clause at the top of
    the file). This function will be accessed by the GUI host using the 
    @c dlsym() function and called to get a LV2UI_UIDescriptor for the
    wanted plugin.
    
    Just like lv2_descriptor(), this function takes an index parameter. The
    index should only be used for enumeration and not as any sort of ID number -
    the host should just iterate from 0 and upwards until the function returns
    NULL, or a descriptor with an URI matching the one the host is looking for
    is returned.
*/
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index);


/** This is the type of the lv2ui_descriptor() function. */
typedef const LV2UI_Descriptor* (*LV2UI_DescriptorFunction)(uint32_t index);



#ifdef __cplusplus
}
#endif


#endif
