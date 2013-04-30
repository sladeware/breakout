#!/usr/bin/env python
#
# Copyright (c) 2013 Sladeware LLC.

import bb.app
from bb.app.hardware.devices.boards import P8X32A_QuickStartBoard
from bb.app.os.drivers.gpio import ButtonDriver
from bb.app.os.drivers.leds import LEDMatrixDriver

board = P8X32A_QuickStartBoard()
breakout = bb.app.Mapping(
  name='breakout',
  #thread_distributor=bb.app.DummyThreadDistributor(),
  processor=board.get_processor(),
  threads=[
    # Add game engine thread
    bb.app.Thread('ENGINE', 'engine_runner', port=bb.app.os.Port(10)),
    # Add button driver thread
    ButtonDriver('BUTTON_DRIVER', port=bb.app.os.Port(10)),
    # Add LED matrix driver thread
    LEDMatrixDriver('LED_MATRIX_DRIVER', port=bb.app.os.Port(10)),
  ]
)
