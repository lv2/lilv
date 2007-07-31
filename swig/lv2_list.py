#!/usr/bin/env python
import slv2;

w = slv2.World()
w.load_all()

plugins = w.get_all_plugins()

for i in range(0, plugins.size()):
    print plugins[i].uri()
