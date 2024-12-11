// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2016 Damien P. George
//
// SPDX-License-Identifier: MIT

#include "common-hal/organio/OrganOut.h"

#include <stdint.h>
#include <stdio.h>

#include "hal/include/hal_gpio.h"

#include "mpconfigport.h"
#include "samd/pins.h"
#include "samd/timers.h"
#include "py/gc.h"
#include "py/runtime.h"
#include "shared-bindings/organio/OrganOut.h"
#include "supervisor/samd_prevent_sleep.h"
#include "timer_handler.h"

#include "shared-bindings/time/__init__.h"

// This timer is shared amongst all PulseOut objects under the assumption that
// the code is single threaded.
static uint8_t refcount = 0;

static uint8_t pulseout_tc_index = 0xff;

static bool tones_running = false;

// TODO: This is stuff that must be compartmentalized, made to be per tone
static digitalio_digitalinout_obj_t *digi_out;

static volatile uint32_t compare_count = 0;


static uint64_t last_toggle = 0;
static uint64_t period_ns;


static uint64_t last_ns = 0xffffffffffffffff;

#define MAX_NUM_DIFFS 10
static uint32_t ns_diffs[MAX_NUM_DIFFS];
static uint32_t diff_ix = 0;

static void pulse_finish(void) {
    if (!tones_running) {
        return;
    }

    bool do_toggle = false;

    uint64_t current_ns = common_hal_time_monotonic_ns();

    if ((last_ns != 0xffffffffffffffff) && (diff_ix < MAX_NUM_DIFFS)) {
        ns_diffs[diff_ix] = (uint32_t)(current_ns - last_ns);
        diff_ix += 1;
    }
    last_ns = current_ns;

    if ((current_ns - last_toggle) > (period_ns / 2)) {
        do_toggle = true;
        last_toggle = current_ns;
    }

    if (do_toggle) {
        bool current_val = common_hal_digitalio_digitalinout_get_value(digi_out);
        common_hal_digitalio_digitalinout_set_value(digi_out, !current_val);
    }

}

void organout_interrupt_handler(uint8_t index) {
    if (index != pulseout_tc_index) {
        return;
    }
    Tc *tc = tc_insts[index];
    if (!tc->COUNT16.INTFLAG.bit.MC0) {
        return;
    }

    pulse_finish();

    // Clear the interrupt bit.
    tc->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
}

void common_hal_organio_organout_construct(organio_organout_obj_t *self,
    const mcu_pin_obj_t *pin,
    uint32_t frequency,
    uint16_t duty_cycle) {

    diff_ix = 0;
    last_ns = 0xffffffffffffffff;

    digitalinout_result_t result = common_hal_digitalio_digitalinout_construct(&self->digi_out, pin);
    if (result != DIGITALINOUT_OK) {
        return;
    }

    // Set to output
    common_hal_digitalio_digitalinout_switch_to_output(&self->digi_out, true, DRIVE_MODE_PUSH_PULL);

    // Set to low
    common_hal_digitalio_digitalinout_set_value(&self->digi_out, false);


    uint32_t period_us = 1000000 / frequency; // TODO: Setting the period timer statically for now.

    /*
    OK, as is, with 48 MHz and NFRQ, we get 1342765 ns => 1.3 ms
    We want approx 4 us, or preferably less.
    Start with 100 us, try to get there.
    Now it's 1342 us, and with my guesses that's with a CC of 0xFFFF (65535), so to get 100, try with
    4883 and 48 MHz.
    */
    if ((duty_cycle & 0x00000001) == 0) {
        compare_count = 1 * 4883; // How fast can you go, little man?
    } else {
        compare_count = 2 * 4883;
    }
    printf("Using CC value %lu for timer\n", compare_count);


    digi_out = &self->digi_out; // TODO: For now, just keep track of our pin in a static variable
    period_ns = period_us * 1000;

    printf("OrganOut init: period_us=%lu, compare_count=%lu\n", period_us, compare_count);

    // TODO: This code is for PulseOut, that has one timer for all instances. I think.
    // The thing I'm working on, well, it COULD be a singleton, but I suppose it'd be possible to
    // do it like they did here – one timer for all instances? Keep your eyes open!
    // TODO: OR, for that matter, maybe the Python API can be like you instantiate one per tone,
    // but that the C code keeps track of it all internally – like this timer? Kinda ugly, though,
    // isn't it? Weird hidden dependency, IDK.
    if (refcount == 0) {
        // Find a spare timer.
        Tc *tc = NULL;
        int8_t index = TC_INST_NUM - 1;
        for (; index >= 0; index--) {
            if (tc_insts[index]->COUNT16.CTRLA.bit.ENABLE == 0) {
                tc = tc_insts[index];
                break;
            }
        }
        if (tc == NULL) {
            mp_raise_RuntimeError(MP_ERROR_TEXT("All timers in use"));
        }

        pulseout_tc_index = index;

        set_timer_handler(true, index, TC_HANDLER_ORGANOUT);
        // We use GCLK0 for SAMD21 and GCLK1 for SAMD51 because they both run at 48mhz making our
        // math the same across the boards.
        #ifdef SAMD21
        turn_on_clocks(true, index, 0);
        #endif
        #ifdef SAM_D5X_E5X

        // TODO: Try other clocks. Try to find the 120 MHz one.
        // It's so hard to understand how a 48 MHz clock with no prescaler could result in such slow
        // action. There is a 32 kHz clock, maybe that's the one that's clock 1? (No, it's not, but
        // still). If no luck with other clocks, clean up the code and post to the forum (Discord?).
        //
        // It is insanely hard to understand. I understand a bit in the ref doc, GCLK section, but
        // cannot map to the code at all.
        // I think this line sets the clock to generator 1 – but the generators, as I understand it,
        // can be configured pretty freely, with different sources, possibility to divide source, etc.
        // I understand it's highly unlikely that something would be wrong in the CircuitPython code
        // base (this code is copied from PulseOut), but I would like to confirm for sure that the
        // clock in fact is 48 MHz. Also, I would actually like to run it on 120 MHz and measure the diff.



/*

Test different variations

1. First of all, run the "current time" thing a couple of times, print the time between calls (i.e.
   approx how long does the call take)

2 And set different combinations of
a) clock (48 & 120)
b) prescaler (1 & 2)
c) compare_count (1 & 2)

a0 b0 c0
a0 b0 c1
a0 b1 c0
a0 b1 c1
a1 b0 c0
a1 b0 c1
a1 b1 c0
a1 b1 c1


And measure time between hits, fill a small array maybe?

Also, wait for sync in all cases, after timer settings, where needed.


*/

        if (duty_cycle < 4) {
            printf("Using 48 MHz clock for timer\n");
            turn_on_clocks(true, index, 1);
        } else {
            printf("Using 120 MHz clock for timer\n");
            turn_on_clocks(true, index, 0);
        }

        #endif


        #ifdef SAMD21
        tc->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 |
            TC_CTRLA_PRESCALER_DIV1 |
            TC_CTRLA_WAVEGEN_NFRQ;
        #endif

        #ifdef SAM_D5X_E5X
        tc_reset(tc);
        tc_set_enable(tc, false);


        printf("Using prescaler DIV1 for timer\n");
        tc->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
        tc_wait_for_sync(tc);

        if ((duty_cycle < 2) || (duty_cycle == 4) || (duty_cycle == 5)) {
            printf("Using WAVE NFRQ for timer\n");
            tc->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_NFRQ;
            tc_wait_for_sync(tc);
        } else {
            printf("Using WAVE MFRQ for timer\n");
            tc->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;
            tc_wait_for_sync(tc);
        }
        #endif

        tc_set_enable(tc, true); // This call does wait for sync in the function
        tc->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP; // Synced when written
    }
    refcount++;

    self->pin = pin->number;

    #ifdef SAMD21
    samd_prevent_sleep();
    #endif
}

bool common_hal_organio_organout_deinited(organio_organout_obj_t *self) {
    return common_hal_digitalio_digitalinout_deinited(&self->digi_out);
}

void common_hal_organio_organout_deinit(organio_organout_obj_t *self) {
    if (common_hal_organio_organout_deinited(self)) {
        return;
    }
    PortGroup *const port_base = &PORT->Group[GPIO_PORT(self->pin)];
    port_base->DIRCLR.reg = 1 << (self->pin % 32);

    common_hal_digitalio_digitalinout_set_value(&self->digi_out, false);

    refcount--;
    if (refcount == 0) {
        tc_reset(tc_insts[pulseout_tc_index]);
        pulseout_tc_index = 0xff;
    }
    self->pin = NO_PIN;
    common_hal_digitalio_digitalinout_deinit(&self->digi_out);
    #ifdef SAMD21
    samd_allow_sleep();
    #endif
}

void common_hal_organio_organout_start(organio_organout_obj_t *self) {
    if (tones_running) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Another organio object is already active"));
    }
    tones_running = true;

    // TODO: At the moment, the timer is set in the constructor – must make it dynamic and
    // clever, but for now, fixed freq and 50% PW.

    Tc *tc = tc_insts[pulseout_tc_index];
    tc->COUNT16.CC[0].reg = compare_count;
    printf("OrganOut start: compare_count=%lu\n", compare_count);

    // Clear our interrupt in case it was set earlier
    tc->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
    tc->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
    tc_enable_interrupts(pulseout_tc_index);

    // TODO: For now, just set our one test pin high, then the timer'll set it low.
    common_hal_digitalio_digitalinout_set_value(&self->digi_out, true);
    last_toggle = common_hal_time_monotonic_ns();

    tc->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
}

void common_hal_organio_organout_stop(organio_organout_obj_t *self) {
    if (!tones_running) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("This organio object is not running"));
    }


    printf("Time diffs: \n");
    for (int i = 0; i < MAX_NUM_DIFFS; i++) {
        printf("%lu\n", ns_diffs[i]);
    }

    Tc *tc = tc_insts[pulseout_tc_index];
    tc->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP;
    tc->COUNT16.INTENCLR.reg = TC_INTENCLR_MC0;
    tc_disable_interrupts(pulseout_tc_index);
    tones_running = false;
}
