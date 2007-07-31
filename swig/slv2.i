%module slv2
%{
#include "../slv2/plugin.h"
#include "../slv2/pluginclass.h"
#include "../slv2/pluginclasses.h"
#include "../slv2/plugininstance.h"
#include "../slv2/plugins.h"
#include "../slv2/port.h"
#include "../slv2/slv2.h"
#include "../slv2/types.h"
#include "../slv2/value.h"
#include "../slv2/values.h"
#include "../slv2/world.h"
typedef struct { SLV2World me; } World;
typedef struct { SLV2World world; SLV2Plugins me; } Plugins;
typedef struct { SLV2World world; SLV2Plugin me; } Plugin;
%}
 
%include "../slv2/plugin.h"
%include "../slv2/pluginclass.h"
%include "../slv2/pluginclasses.h"
%include "../slv2/plugininstance.h"
%include "../slv2/plugins.h"
%include "../slv2/port.h"
%include "../slv2/slv2.h"
%include "../slv2/types.h"
%include "../slv2/value.h"
%include "../slv2/values.h"
%include "../slv2/world.h"

typedef struct { SLV2Plugin me; } Plugin;
%extend Plugin {
    Plugin(SLV2Plugin p) {
        Plugin* ret = malloc(sizeof(Plugin));
        ret->me = p;
        return ret;
    }
    
    ~Plugin() {
        /* FIXME: free SLV2Plugin? */
        free($self);
    }

    char* name() { return slv2_plugin_get_name($self->me); }
    const char* uri() { return slv2_plugin_get_uri($self->me); }
};

typedef struct { SLV2Plugins me; } Plugins;
%extend Plugins {
    Plugins(SLV2World w, SLV2Plugins p) {
        Plugins* ret = malloc(sizeof(Plugins));
        ret->world = w;
        ret->me = p;
        return ret;
    }

    ~Plugins() {
        slv2_plugins_free($self->world, $self->me);
        free($self);
    }

    inline unsigned size() const { return slv2_plugins_size($self->me); }
    inline unsigned __len__() const { return slv2_plugins_size($self->me); }

    Plugin* __getitem__(unsigned i) {
        if (i < slv2_plugins_size($self->me))
            return new_Plugin(slv2_plugins_get_at($self->me, i));
        else
            return NULL;
    }
};

typedef struct { SLV2World me; } World;
%extend World {
    World() {
        World* ret = malloc(sizeof(World));
        ret->me = slv2_world_new();
        return ret;
    }
    
    ~World() {
        slv2_world_free($self->me);
        free($self);
    }
    /*World() { $self->me = slv2_world_new(); }
    ~World() { slv2_world_free($self->me); }*/

    void load_all() { slv2_world_load_all($self->me); }
    void load_bundle(const char* path) { slv2_world_load_bundle($self->me, path); }
    Plugins* get_all_plugins() { return new_Plugins($self->me, slv2_world_get_all_plugins($self->me)); }
};


