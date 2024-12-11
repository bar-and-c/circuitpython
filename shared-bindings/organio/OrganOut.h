// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2013, 2014 Damien P. George
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common-hal/microcontroller/Pin.h"
#include "common-hal/organio/OrganOut.h"
#include "common-hal/pwmio/PWMOut.h"

extern const mp_obj_type_t organio_organout_type;

extern void common_hal_organio_organout_construct(organio_organout_obj_t *self,
    const mcu_pin_obj_t *pin,
    uint32_t frequency,
    uint16_t duty_cycle);

extern void common_hal_organio_organout_deinit(organio_organout_obj_t *self);
extern bool common_hal_organio_organout_deinited(organio_organout_obj_t *self);
extern void common_hal_organio_organout_start(organio_organout_obj_t *self);
extern void common_hal_organio_organout_stop(organio_organout_obj_t *self);
