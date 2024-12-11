// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2016 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/organio/__init__.h"
#include "shared-bindings/organio/OrganOut.h"

//| """Support for individual pulse based protocols
//|
//| The `organio` module contains classes to provide access to basic pulse IO.
//| Individual pulses are commonly used in infrared remotes and in DHT
//| temperature sensors.
//|
//| All classes change hardware state and should be deinitialized when they
//| are no longer needed if the program continues after use. To do so, either
//| call :py:meth:`!deinit` or use a context manager. See
//| :ref:`lifetime-and-contextmanagers` for more info."""

static const mp_rom_map_elem_t organio_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_organio) },
    { MP_ROM_QSTR(MP_QSTR_OrganOut), MP_ROM_PTR(&organio_organout_type) },
};

static MP_DEFINE_CONST_DICT(organio_module_globals, organio_module_globals_table);

const mp_obj_module_t organio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&organio_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_organio, organio_module);
