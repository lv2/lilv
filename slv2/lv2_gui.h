/************************************************************************
 *
 * In-process UI extension for LV2
 *
 * Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
 * 
 * Based on lv2.h, which is:
 *
 * Copyright (C) 2000-2002 Richard W.E. Furse, Paul Barton-Davis, 
 *                         Stefan Westerfeld
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
    hosts to create GUIs for plugins. The GUIs are plugins that reside in
    shared object files in an LV2 bundle and be referenced in the RDF file
    using the triples (Turtle shown)
<pre>    
    @prefix guiext: <http://ll-plugins.nongnu.org/lv2/ext/ipgui/1#> .
    <http://my.plugin>    guiext:gui    <http://my.plugingui> .
    <http://my.plugingui> guiext:binary <mygui.so> .
</pre>
    where <http://my.plugin> is the URI of the plugin, <http://my.plugingui> is
    the URI of the plugin GUI and <mygui.so> is the relative URI to the shared 
    object file. While it is possible to have the plugin GUI and the plugin in 
    the same shared object file it is probably a good idea to keep them 
    separate so that hosts that don't want GUIs don't have to load the GUI code.
    
    (Note: the prefix above is used throughout this file for the same URI)
    
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
    functions defined below may only be called in the same thread as the UI
    main loop is running in.
*/

#ifndef LV2_GUI_H
#define LV2_GUI_H

#include "lv2.h"


#ifdef __cplusplus
extern "C" {
#endif


/** A pointer to some widget.
 * The actual type of the widget is defined by the type URI of the GUI.
 * e.g. if "<http://example.org/somegui> a guiext:GtkGUI", this is a pointer
 * to a GtkWidget.  All the functionality provided by this extension is
 * toolkit independent, the host only needs to pass the necessary callbacks
 * and display the widget, if possible.  Plugins may have several GUIs,
 * in various toolkits.
 */
typedef void* LV2UI_Widget;

  
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


/** This is the type of the host-provided function that the GUI can use to
    send data to a plugin's input ports. The @c buffer parameter must point
    to a block of data, @c buffer_size bytes large. The contents of this buffer
    will depend on the class of the port it's being sent to. For ports of the
    class lv2:ControlPort, buffer_size should be sizeof(float) and the buffer 
    contents should be a float value. For ports of the class llext:MidiPort the
    buffer should contain the data bytes of a single MIDI event, and buffer_size
    should be the number of bytes in the event. No other port classes are
    allowed, unless the format and the meaning of the buffer passed to this 
    function is defined in the extension that specifies that class or in a 
    separate GUI host feature extension that is required by this GUI.
    
    The GUI is responsible for allocating the buffer and deallocating it after
    the call. There are no timing guarantees at all for this function, although
    the faster the host can get the data to the plugin port the better. A 
    function pointer of this type will be provided to the GUI by the host in
    the instantiate() function. */
typedef void (*LV2UI_Write_Function)(LV2UI_Controller controller,
                                     uint32_t         port_index,
                                     uint32_t         buffer_size,
                                     const void*      buffer);

/** This is the type of the host-provided function that the GUI can use to
    send arbitrary commands to the plugin. The parameters after the first one
    should be const char* variables, terminated by a NULL pointer, and will be
    interpreted as a command with arguments. A function of this type will be 
    provided to the GUI by the host in the instantiate() function. */
typedef void (*LV2UI_Command_Function)(LV2UI_Controller   controller,
                                       uint32_t           argc,
                                       const char* const* argv);

/** This is the type of the host-provided function that the GUI can use to
    request a program change in the host. A function of this type will be 
    provided to the GUI by the host in the instantiate() function. Calling
    this function does not guarantee that the program will change, it is just
    a request. If the program does change, the GUI's current_program_changed() 
    callback will be called, either before or after this function returns
    depending on whether the GUI host <-> plugin instance communication is
    synchronous or asynchronous. */
typedef void (*LV2UI_Program_Change_Function)(LV2UI_Controller controller,
                                              unsigned char    program);

/** This is the type of the host-provided function that the GUI can use to
    request that the current state of the plugin is saved to a program.
    A function of this type will be provided to the GUI by the host in the
    instantiate function. Calling this function does not guarantee that the
    state will be saved, it is just a request. If the state is saved, the 
    GUI's program_added() callback will be called, either before or after
    this function returns depending on whether the GUI host <-> plugin
    instance communication is synchronous or asynchronous. */
typedef void (*LV2UI_Program_Save_Function)(LV2UI_Controller controller,
                                            unsigned char    program,
                                            const char*      name);


/** */
typedef struct _LV2UI_Descriptor {
  
  /** The URI for this GUI (not for the plugin it controls). */
  const char* URI;
  
  /** Create a new GUI object and return a handle to it. This function works
      similarly to the instantiate() member in LV2_Descriptor, with the 
      additions that the URI for the plugin that this GUI is for is passed
      as a parameter, a function pointer and a controller handle are passed to
      allow the plugin to write to input ports in the plugin (write_function 
      and controller) and a pointer to a LV2_Widget is passed, which the GUI
	  plugin should set to point to a newly created widget which will be the
	  GUI for the plugin.  This widget may only be destroyed by cleanup().
      
      The features array works like the one in the instantiate() member
      in LV2_Descriptor, except that the URIs should be denoted with the triples
      <pre>
      <http://my.plugingui> guiext:optionalFeature <http://my.guifeature>
      </pre>
      or 
      <pre>
      <http://my.plugingui> guiext:requiredFeature <http://my.guifeature>
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
                              LV2UI_Write_Function            write_function,
                              LV2UI_Command_Function          command_function,
                              LV2UI_Program_Change_Function   program_function,
                              LV2UI_Program_Save_Function     save_function,
                              LV2UI_Controller                controller,
                              LV2UI_Widget*                   widget,
                              const LV2_Host_Feature* const*  host_features);

  
  /** Destroy the GUI object and the associated widget. 
   */
  void (*cleanup)(LV2UI_Handle gui);
  
  /** Tell the GUI that something interesting has happened at a plugin port.
      For control ports this would be when the value in the buffer has changed,
      for message-based port classes like MIDI or OSC it would be when a 
      message has arrived in the buffer. For other port classes it is not 
      defined when this function is called, unless it is specified in the 
      definition of that port class extension. For control ports the default
      setting is to call this function whenever an input control port value 
      has changed but not when any output control port value has changed, for 
      all other port classes the default setting is to never call this function.

      However, the default setting can be modified by using the following
      URIs:
      <pre>
      guiext:portNotification
      guiext:noPortNotification
      guiext:plugin
      guiext:portIndex
      </pre>
      For example, if you want the GUI with uri 
      <code><http://my.plugingui></code> for the plugin with URI 
      <code><http://my.plugin></code> to get notified when the value of the 
      output control port with index 4 changes, you would use the following 
      in the RDF for your GUI:
      <pre>
      <http://my.plugingui> guiext:portNotification [ guiext:plugin <http://my.plugin> ;
                                                      guiext:portIndex 4 ] .
      </pre>
      and similarly with <code>guiext:noPortNotification</code> if you wanted to 
      prevent notifications for a port for which it would be on by default 
      otherwise.
      
      The @c buffer is only valid during the time of this function call, so if 
      the GUI wants to keep it for later use it has to copy the contents to an
      internal buffer.
      
      The buffer is subject to the same rules as the ones for the 
      LV2_Write_Function type. This means that a plugin GUI may not request a
      portNotification for a port that has a class other than lv2:ControlPort
      or llext:MidiPort unless the buffer format and meaning is specified in
      the extension that defines that port class, or in a separate GUI host 
      feature extension that is required by the GUI. Any GUI that does that
      should be considered broken and the host should not use it.
      
      This member may be set to NULL if the GUI is not interested in any 
      port events.
  */
  void (*port_event)(LV2UI_Handle   gui,
                     uint32_t       port,
                     uint32_t       buffer_size,
                     const void*    buffer);
  
  /** This function is called when the plugin instance wants to send feedback
      to the GUI. It may be called in response to a command function call,
      either before or after the command function has returned (depending on
      whether the GUI host <-> plugin instance communication is synchronous or
      asynchronous). */
  void (*feedback)(LV2UI_Handle       gui, 
                   uint32_t           argc, 
                   const char* const* argv);
  
  /** This function is called when the host adds a new program to its program
      list, or changes the name of an old one. It may be set to NULL if the 
      GUI isn't interested in displaying program information. */
  void (*program_added)(LV2UI_Handle  gui, 
                        unsigned char number, 
                        const char*   name);
  
  /** This function is called when the host removes a program from its program
      list. It may be set to NULL if the GUI isn't interested in displaying
      program information. */
  void (*program_removed)(LV2UI_Handle  gui, 
                          unsigned char number);
  
  /** This function is called when the host clears its program list. It may be
      set to NULL if the GUI isn't interested in displaying program 
      information. */
  void (*programs_cleared)(LV2UI_Handle gui);
  
  /** This function is called when the host changes the current program. It may
      be set to NULL if the GUI isn't interested in displaying program 
      information. */
  void (*current_program_changed)(LV2UI_Handle  gui, 
                                  unsigned char number);

  /** Returns a data structure associated with an extension URI, for example
      a struct containing additional function pointers. Avoid returning
      function pointers directly since standard C++ has no valid way of
      casting a void* to a function pointer. This member may be set to NULL
      if the GUI is not interested in supporting any extensions. This is similar
      to the extension_data() member in LV2_Descriptor.
  */
  void* (*extension_data)(LV2UI_Handle gui,
                          const char*  uri);

} LV2UI_Descriptor;



/** A plugin programmer must include a function called "lv2ui_descriptor"
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


#endif /* LV2_GUI_H */

