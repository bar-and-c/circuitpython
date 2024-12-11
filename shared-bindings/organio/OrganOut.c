// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "shared/runtime/context_manager_helpers.h"
#include "py/objproperty.h"
#include "py/runtime.h"

#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/organio/OrganOut.h"
#include "shared-bindings/util.h"

/*

TODO:

How shall this work?

Basically, You instantiate OrganOut, tell it on which pins you want which frequencies, and then it
just toggles those pins high/low until it's stopped.

Somehow it should be possible to tell it to do vibrato and PWM – definitely on a global level, but
ideally also individually (either with some kind of "spread" level, effectively a factor saying how
much a tone should be affected by the vibrato level, or by actually implementing one LFO per tone).

Maybe this class should handle that as well, in which case it might as well be called OrganIO.

The OrganOut class should run a timer – either periodically, like a sample rate, or by checking all
tones, finding the next one to toggle, and set a timer for that time.

There is not much else to do, I suppose, on the board. Someone (maybe an OrganIn?) should read some
optional analog pins to set PWM, vibrato, pulse width (or perhaps PWM is done from the outside, then
only PW in)?

Other than that I don't think it'll do a lot.
What I was thinking was this:
The static "sample rate" timer sounds like it might introduce more jitter (for low sample rates),
but probably be less intrusive on the system.
Setting the timer for "next toggle" might be more precise, but if there are a lot of high frequency
tones there would be instances when there's just too much going on (as in the time until the next
toggle might be so short that the timer lacks resolution to handle it, and then when it does trigger,
there are overdue timers, etc.).

I'm guessing now, but I think a static sample rate might be better.

NOTE: There might be more stuff going on in the future – for instance, the pins on the Grand Central
are not enough, I think, in that case there will be one more board for sure. In that case it could
handle CV/MIDI stuff from the lower manual.
But I mean, it's an 120 Mhz processor. We're talking about a sample rate to be able to handle tones
up to 2.5 kHz. But sure, it's not sample rate as such...or is it exactly what it is?
???
At first I thought that when you loop at say 25000 Hz, you check if the 2500 Hz tone should toggle
or not. The tone has a period of 400 us (will toggle in average every 200 us). The "sample rate" has
a period of 40 us; every 40 us it'll check if tone X should toggle or not, and if only checking "is
it overdue" the tone period will be 400 us when we're extremely lucky, and 440 us when extremely
unlucky, so there ought to be periodic glitches where its frequency hops from 2272 Hz to 2500 Hz.
That is A LOT. Sure, we could detect if any tone has less time left until its next toggle than the
assumed next sample tick, but I suppose that would only cut the glitch in half, or whatever, but still.

On the other hand – how is this different from real sampling? In real sampling we'd be sampling at
this 25000 Hz rate, and the audio we're sampling is a square wave of 2500 Hz – at some points the
sample tick should hit exactly when the signal has toggled, and at the worst case scenario it'd hit
40 us later...right? But sure, when saying you can sample frequencies up to half the sample rate,
that's stretching it a bit. As I recall, there are some tricks done to reproduce, isn't there?
- TALK TO ZOLTAN!
- TEST STUFF!

In either case – it's 120 Mhz! Even if we'd run the loop at 120 kHz there ought to be a lot of time
left for it to work, as it's not doing much.


========================== DEV PLAN ==========================

How to move forward?

Try to make OrganOut just toggle a pin so that the LED can display it (i.e. slowly).

TODO:
- PulseOut uses PwmOut, I think – we'd like to use DigitalOut. See if maybe PwmOut uses that, or
  anyone else, and understand how.
- Maybe just init it to a fixed freq/PW?
*/


//| class OrganOut:
//|     """Pulse PWM-modulated "carrier" output on and off. This is commonly used in infrared remotes. The
//|     pulsed signal consists of timed on and off periods. Unlike `pwmio.PWMOut`, there is no set duration
//|     for on and off pairs."""
//|
//|     def __init__(
//|         self, pin: microcontroller.Pin, *, frequency: int = 38000, duty_cycle: int = 1 << 15
//|     ) -> None:
//|         """Create a OrganOut object associated with the given pin.
//|
//|         :param ~microcontroller.Pin pin: Signal output pin
//|         :param int frequency: Carrier signal frequency in Hertz
//|         :param int duty_cycle: 16-bit duty cycle of carrier frequency (0 - 65536)
//|
//|         Send a short series of pulses::
//|
//|           import array
//|           import organio
//|           import board
//|
//|           # 50% duty cycle at 38kHz.
//|           pulse = organio.OrganOut(board.LED, frequency=38000, duty_cycle=32768)
//|           #                             on   off     on    off    on
//|           pulses = array.array('H', [65000, 1000, 65000, 65000, 1000])
//|           pulse.send(pulses)
//|
//|           # Modify the array of pulses.
//|           pulses[0] = 200
//|           pulse.send(pulses)"""
//|         ...
static mp_obj_t organio_organout_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    // TODO: Either let the constructor take an array of this (pin/freq/PW) or have an "add()" method
    #if CIRCUITPY_ORGANIO
    enum { ARG_pin, ARG_frequency, ARG_duty_cycle};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_pin, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_frequency, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 38000} },
        { MP_QSTR_duty_cycle, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1 << 15} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const mcu_pin_obj_t *pin = validate_obj_is_free_pin(args[ARG_pin].u_obj, MP_QSTR_pin);
    mp_int_t frequency = args[ARG_frequency].u_int;
    mp_int_t duty_cycle = args[ARG_duty_cycle].u_int;

    organio_organout_obj_t *self = m_new_obj_with_finaliser(organio_organout_obj_t);
    self->base.type = &organio_organout_type;
    common_hal_organio_organout_construct(self, pin, frequency, duty_cycle);
    return MP_OBJ_FROM_PTR(self);
    #else
    mp_raise_NotImplementedError(NULL);
    #endif
}

#if CIRCUITPY_ORGANIO
//|     def deinit(self) -> None:
//|         """Deinitialises the OrganOut and releases any hardware resources for reuse."""
//|         ...
static mp_obj_t organio_organout_deinit(mp_obj_t self_in) {
    organio_organout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_organio_organout_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(organio_organout_deinit_obj, organio_organout_deinit);

//|     def __enter__(self) -> OrganOut:
//|         """No-op used by Context Managers."""
//|         ...
//  Provided by context manager helper.

//|     def __exit__(self) -> None:
//|         """Automatically deinitializes the hardware when exiting a context. See
//|         :ref:`lifetime-and-contextmanagers` for more info."""
//|         ...
static mp_obj_t organio_organout_obj___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_organio_organout_deinit(args[0]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(organio_organout___exit___obj, 4, 4, organio_organout_obj___exit__);

//|     def start(self) -> None:
//|         """Start the tone generation"""
//|         ...
//|
static mp_obj_t organio_organout_obj_start(mp_obj_t self_in) {
    organio_organout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (common_hal_organio_organout_deinited(self)) {
        raise_deinited_error();
    }

    common_hal_organio_organout_start(self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(organio_organout_start_obj, organio_organout_obj_start);


//|     def stop(self, pulses: ReadableBuffer) -> None:
//|         """Stop the tone generation"""
//|         ...
//|
static mp_obj_t organio_organout_obj_stop(mp_obj_t self_in) {
    organio_organout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (common_hal_organio_organout_deinited(self)) {
        raise_deinited_error();
    }

    common_hal_organio_organout_stop(self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(organio_organout_stop_obj, organio_organout_obj_stop);

#endif // CIRCUITPY_ORGANIO

static const mp_rom_map_elem_t organio_organout_locals_dict_table[] = {
    // Methods
    #if CIRCUITPY_ORGANIO
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&organio_organout_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&organio_organout_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&organio_organout___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&organio_organout_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&organio_organout_stop_obj) },
    #endif // CIRCUITPY_ORGANIO
};
static MP_DEFINE_CONST_DICT(organio_organout_locals_dict, organio_organout_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    organio_organout_type,
    MP_QSTR_OrganOut,
    MP_TYPE_FLAG_NONE,
    make_new, organio_organout_make_new,
    locals_dict, &organio_organout_locals_dict
    );
