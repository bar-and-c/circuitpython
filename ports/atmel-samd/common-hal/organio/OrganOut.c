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


/*

MULTIPLE TONES

New method add(), to add a tone/pin/freq thing – e.g. add(frequency, pin).
A struct to keep the necessary stuff in.
Keep an array of such structs, fill it in with add().
In the interrupt handler, do what's done now, but iterate over the array.

FUTURE:

Add methods (or some kind of properties?) for vibrato and PWM, maybe something like:

set_vibrato_freq(frequency, spread) (where "spread" is a float 0..1 stating how much the different
                                     tones' vibrato shall differ (maybe random, not spread, idk)).

set_vibrato_depth()

*/

/*
THE NEXT STEP IN THIS ADVENTURE!

I don't think I can use common_hal_time_monotonic_ns() to calculate period time,
since it seems to increment in steps of approx. 122072 ns, i.e. 0.1 ms, which is
way too high (the shortest period is 1/2500, i.e. 0.4 ms).

The only way I can see that could make this work is to rely on the timer increments.
Set a compare_count to say 100, meaning that for each interrupt handler call there'd
be (ideally!) 100 MCU cycles to do work in. I don't think that's enough, especially
when weighing in PWM, vibrato, et cetera.

Maybe no point in trying?

But OK, if it DOES work, maybe do similar experiments to this one, but instead of
comparing different prescaler values et cetera, compare how the highest tone sounds
when 10, 30, 50 other tones.

And if that's OK, with PWM etc. in place, compare similarly – 10, 30, 50 tones with
PWM and vibrato, se how much it can take.

Most likely, I think I will have to fall back to what's working, i.e. pwmio and 12
tones per board.
OR if I can manage to get Beaglebone with PRU running (or bare metal, but I think
PRU is a better bet).


SO, what is step one?

1. Try different compare_count values, how high can you go?

a) Define a static compare_count and interrupt_time_ns (or similar). In the future
   these can be constants even (one set for 48 MHz on SAMD21, one for 120 MHz).

b) In the interrupt handler, reset the timer immediately (to get the best possible
   accuracy – at the risk of everything going to hell if too much time is spent there
   or if too much other stuff happens).

c) Then – how to calculate time spent? Maybe just define a tone's "next toggle" etc. by
   tick cycles? I.e. each IRQ call, increment a XX bit counter. When deining a tone, set
   its period as the number of the number of such e.g. 1.6 us cycles (1.6 us is for 120
   MHz clocked timer with 200 CC, I think).

d) Set up something similar to now, with different compare counts for different "duty_cycle".


How wide a cycle counter?
Say a cycle takes 1.6 us.
Then the cycle_counter gets incremented every 1.6 us.
A 32 bit counter will wrap in 2^32 cycles, i.e. in 2^32 * 1.6 us.

Well, a 32 bit counter wraps in 35 seconds. Disappointing.
And a 64 bit counter? More like in thousands of years. Is that possible?

120000000 = ca 2^27
32 bit: 2^32 / 2^27 = 2^5 = 32 seconds
64 bit: 2^64 / 2^27 = 2^37 = 4358 years



First, just do one tone.

*/



/*
>>>>>>>>>>>>>>>>>>>>  OTHER MAD IDEA!

If relying on the timer, maybe possible to do the first variant, i.e. set the timer to the next
toggle in time?

>>>>>>>>>>>>>>>>>>>>
*/

/* The reset time for the main timer – too low a value makes things weird, but essentially it's the
   sample rate, so higher frequencies (and PWM/vibrato) will be better with lower values. Experiment
   with how high/low we can go without things becoming audibly bad or causing the board to hang etc.
   On SAMD51, running the timer on 120 MHz, this value is approx. "how many periods of 120 MHz" – e.g.
   the value 400 means 400 / 120000000 => 3.3 us. At least that's how I _think_ it works. :-) */
#define ORGANIO_TIMER_CC 400

static volatile uint16_t compare_count = 0;
static uint64_t cycle_counter;
static uint32_t cycle_time_ns;


// This timer is shared amongst all PulseOut objects under the assumption that
// the code is single threaded.
static uint8_t refcount = 0;

static uint8_t pulseout_tc_index = 0xff;

static bool tones_running = false;



/* OK, tried to put this in "self", because it seemed sane – keep the timer handling details in the
   C file as statics, but put the tone config in the "object" – but I just couldn't figure out how
   to get the tone array in the interrupt handler, there is no concept of "self" there.
   So, keeping them here as statics. */
#define MAX_NUM_TONES 56 // TODO: What IS max? Also, it would be board dependent, amirite?

struct tone {
    uint8_t pin;
    digitalio_digitalinout_obj_t digi_out;
    mp_float_t frequency;
    uint64_t period_cycles;
    uint64_t last_toggle_cycle;
};

static struct tone tones[MAX_NUM_TONES];
static size_t tones_end_ix;


static void common_hal_organio_organout_add(organio_organout_obj_t *self,
                                            const mcu_pin_obj_t *pin,
                                            const mp_float_t frequency);


static bool time_to_toggle(uint64_t current_cycle, struct tone *tone) {
    // TODO: Make it cope with wrapping (and then make cycle_counter 32-bit);
    // the former because for form's sake I think it'd be nice to make it possible
    // to instantiate several organio, and the latter because if it can handle wrapping
    // why bother with 64-bit (on the other hand, why not).
    // In that scenario, also maybe just start the timer when the first organio is initialized?
    // Well, NO, NO, NO – the ideas above stem from a confusion about PulseOut, from where this
    // code is stolen. In PulseOut, yes, all instances share a timer, BUT they can only "send()"
    // one at a time. For this to work in this case, I'd have to 1) keep the tone arrays in self,
    // and 2) keep track of the different instances that are started. For now, let's just make it
    // a singleton.
    return ((current_cycle - tone->last_toggle_cycle) > (tone->period_cycles / 2));
}

static void pulse_finish(void) {
    if (!tones_running) {
        return;
    }

    bool do_toggle = false;

    cycle_counter += 1;

    for (size_t i = 0; i < tones_end_ix; i++) {
        if (time_to_toggle(cycle_counter, &tones[i])) {
            do_toggle = true;
            tones[i].last_toggle_cycle = cycle_counter;
        }

        if (do_toggle) {
            bool current_val = common_hal_digitalio_digitalinout_get_value(&tones[i].digi_out);
            common_hal_digitalio_digitalinout_set_value(&tones[i].digi_out, !current_val);
        }
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

    // TODO: See if moving this to the top makes a difference (could theorethically make the timing better)
    // Clear the interrupt bit.
    tc->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
}

// TODO: As it looks now, pin/freq/DC will not be set in this init, but in an add() – remove when the dust settles
void common_hal_organio_organout_construct(organio_organout_obj_t *self,
    const mcu_pin_obj_t *pins[],
    const mp_float_t frequencies[],
    const size_t num_tones) {

    cycle_counter = 0;
    memset(tones, 0, sizeof(tones));
    tones_end_ix = 0;

    compare_count = ORGANIO_TIMER_CC;
    printf("Using CC value %u for timer\n", compare_count);

    cycle_time_ns = 1000 * compare_count / 120; // Skipped six zeroes in the first/last term
    printf("OrganOut init: cycle_time_ns=%lu, compare_count=%u\n", cycle_time_ns, compare_count);


    /* For now only supporting one instance – the intended usage is to instantiate one organio with
       all the tones you need, organio will handle ALL your organ needs, so it's not like this
       restriction really is a problem – it's how it's used. And it's all for me, anyway. */
    if (refcount > 0) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("For now, organio can only be instantiated once – deinit and retry"));
    }

    tones_end_ix = 0;
    printf("TODO: sizeof(self->tones)=%d\n", sizeof(tones));
    memset(tones, 0, sizeof(tones));

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

        // Always use 120 MHz on SAMD51
        // TODO: Make SAMD21 work later, if needed
        printf("Using 120 MHz clock for timer\n");
        turn_on_clocks(true, index, 0);

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

        // Always use MFRQ, because we want to have variable times (couldn't make that work with NFRQ, so sue me)
        printf("Using WAVE MFRQ for timer\n");
        tc->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;
        tc_wait_for_sync(tc);
        #endif

        tc_set_enable(tc, true); // This call does wait for sync in the function
        tc->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP; // Synced when written
    }
    refcount++;

    for (size_t i = 0; i < num_tones; i++) {
        common_hal_organio_organout_add(self, pins[i], frequencies[i]);
    }

    #ifdef SAMD21
    samd_prevent_sleep();
    #endif
}

bool common_hal_organio_organout_deinited(organio_organout_obj_t *self) {
    for (size_t i = 0; i < tones_end_ix; i++) {
        if (!common_hal_digitalio_digitalinout_deinited(&tones[i].digi_out)) {
            return false;
        }
    }
    return true;
}

void common_hal_organio_organout_deinit(organio_organout_obj_t *self) {
    if (common_hal_organio_organout_deinited(self)) {
        return;
    }

    for (size_t i = 0; i < tones_end_ix; i++) {
        PortGroup *const port_base = &PORT->Group[GPIO_PORT(tones[i].pin)];
        port_base->DIRCLR.reg = 1 << (tones[i].pin % 32);

        common_hal_digitalio_digitalinout_set_value(&tones[i].digi_out, false);
        tones[i].pin = NO_PIN;
        common_hal_digitalio_digitalinout_deinit(&tones[i].digi_out);
    }

    refcount--;
    if (refcount == 0) {
        tc_reset(tc_insts[pulseout_tc_index]);
        pulseout_tc_index = 0xff;
    }
    #ifdef SAMD21
    samd_allow_sleep();
    #endif
}

static void common_hal_organio_organout_add(organio_organout_obj_t *self,
                                            const mcu_pin_obj_t *pin,
                                            const mp_float_t frequency) {

    if (tones_end_ix == MAX_NUM_TONES) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Reached max number of tones"));
    }

    tones[tones_end_ix].pin = pin->number;
    tones[tones_end_ix].frequency = frequency;
    struct tone *current_tone = &tones[tones_end_ix];
    tones_end_ix += 1;


    // Init digital pin, set to output and low
    digitalinout_result_t result = common_hal_digitalio_digitalinout_construct(&current_tone->digi_out, pin);
    if (result != DIGITALINOUT_OK) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Could not instantiate pin as digital out"));
    }
    common_hal_digitalio_digitalinout_switch_to_output(&current_tone->digi_out, true, DRIVE_MODE_PUSH_PULL);
    common_hal_digitalio_digitalinout_set_value(&current_tone->digi_out, false);

    // Set frequency stuff
    uint32_t period_ns = 1000000000 / frequency;

    current_tone->period_cycles = period_ns / cycle_time_ns;
    current_tone->last_toggle_cycle = 0;

    printf("OrganOut add: period_ns=%lu, period_cycles top=%lu, bottom=%lu\n", period_ns, (uint32_t)(current_tone->period_cycles>>32), (uint32_t)(current_tone->period_cycles & 0xffffffff));
}

void common_hal_organio_organout_start(organio_organout_obj_t *self) {
    if (tones_running) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Another organio object is already active"));
    }
    tones_running = true;

    // TODO: At the moment, the timer is set in the constructor – must make it dynamic and
    // clever, but for now, fixed freq and 50% PW.

    Tc *tc = tc_insts[pulseout_tc_index];

    // TODO: No need to set that here now
    tc->COUNT16.CC[0].reg = compare_count;
    printf("OrganOut start: compare_count=%u\n", compare_count);

    // Clear our interrupt in case it was set earlier
    tc->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
    tc->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
    tc_enable_interrupts(pulseout_tc_index);


    cycle_counter = 0;
    for (size_t i = 0; i < tones_end_ix; i++) {
        tones[i].last_toggle_cycle = 0;
    }

    tc->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
}

void common_hal_organio_organout_stop(organio_organout_obj_t *self) {
    if (!tones_running) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("This organio object is not running"));
    }

    Tc *tc = tc_insts[pulseout_tc_index];
    tc->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP;
    tc->COUNT16.INTENCLR.reg = TC_INTENCLR_MC0;
    tc_disable_interrupts(pulseout_tc_index);
    tones_running = false;

    for (size_t i = 0; i < tones_end_ix; i++) {
        common_hal_digitalio_digitalinout_set_value(&tones[i].digi_out, false);
    }
}
