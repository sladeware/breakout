# -*- mode: python; -*-
#
# Copyright (c) 2013 Sladeware LLC.

from breakout import breakout

propeller_binary(name="breakout",
                 srcs=["src/engine.c", breakout])

propeller_load(name="breakout-load",
               binary="breakout",
               deps=[":breakout"],
               baudrate=115200,
               terminal_mode=False,
               port="/dev/ttyUSB0",
               eeprom=False)
