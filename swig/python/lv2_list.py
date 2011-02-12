#!/usr/bin/env python

import slv2

world = slv2.World()
world.load_all()

plugins = world.get_all_plugins()

for i in plugins:
	print(i.get_uri().as_uri())

