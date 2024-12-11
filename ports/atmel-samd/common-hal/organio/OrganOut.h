// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common-hal/microcontroller/Pin.h"

#include "py/obj.h"
#include "shared-bindings/digitalio/DigitalInOut.h"

typedef struct {
    mp_obj_base_t base;
    digitalio_digitalinout_obj_t digi_out;
    uint8_t pin;
} organio_organout_obj_t;

void organout_reset(void);
void organout_interrupt_handler(uint8_t index);
