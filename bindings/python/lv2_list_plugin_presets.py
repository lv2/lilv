#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""List all presets of an LV2 plugin with the given URI."""

import sys
import lilv


LILV_NS_PRESETS = 'http://lv2plug.in/ns/ext/presets#'


def main(args=None):
    args = sys.argv[1:] if args is None else args

    if args:
        uri = args[0]
    else:
        return "Usage: lv2_list_plugin_presets <plugin URI>"

    world = lilv.World()
    world.load_all()
    world.ns.presets = lilv.Namespace(world, LILV_NS_PRESETS)
    plugins = world.get_all_plugins()

    try:
        plugin_uri = world.new_uri(uri)
    except ValueError:
        return "Invalid URI '%s'." % uri

    try:
        plugin = plugins[plugin_uri]
    except KeyError:
        return "Plugin with URI '%s' not found." % uri

    presets = plugin.get_related(world.ns.presets.Preset)
    preset_list = []

    for preset in presets:
        world.load_resource(preset)
        labels = world.find_nodes(preset, world.ns.rdfs.label, None)

        if labels:
            label = str(labels[0])
        else:
            label = str(preset)
            print("Preset '%s' has no rdfs:label" % preset, file=sys.stderr)

        preset_list.append((label, str(preset)))

    for preset in sorted(preset_list):
        print("Label: %s\nURI: %s\n" % preset)


if __name__ == '__main__':
    sys.exit(main() or 0)
