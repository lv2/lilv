#!/usr/bin/env python

import slv2

world = slv2.World()
world.load_all()

for i in world.get_all_plugins():
    print(i.get_uri())
