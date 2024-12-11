#
# This file is part of the MicroPython project, http://micropython.org/
#
# The MIT License (MIT)
#
# SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Boards default to all modules enabled (with exceptions)
# Manually disable by overriding in #mpconfigboard.mk

# These Makefile variables are used to implement the "any" and "all" functions.
# Note that these only work when the arguments expand to "0" and/or "1" but not
# if they expand to other values like "yes", "/bin/sh", or "false".
#
# Make's "sort" will transform a mixed sequence of 0s and 1s to "0 1" (because
# it also eliminates duplicates), or a non-mixed sequence of "0" or "1" to just
# itself. Thus, if all the inputs are 1 then the first word will be 1; if any
# of the inputs are 1, then the last word will be 1.
enable-if-any=$(lastword $(sort $(1) 0))
enable-if-all=$(firstword $(sort $(1) 1))

# Implement the 'not' function; When first argument argument of $(if) is non-empty,
# the result is the 2nd arg, else it's the third arg. So if the value of $(1)
# is exactly 1, the result of $(filter) is empty, and the result is 0. Otherwise,
# the result is 1. You use this by $(call)ing it.
enable-if-not=$(if $(filter 1,$(1)),0,1)

# To use any/all, you "$(call)" it, with the values to test after a comma.
# Usually the values are other $(CIRCUITPY_foo) variables. The definition
# of CIRCUITPY_AUDIOCORE and CIRCUITPY_AUDIOMP3 below are typical of how
# any/all are expected to be used.

# Always on. Present here to help generate documentation module support matrix for "builtins".
CIRCUITPY = 1
CFLAGS += -DCIRCUITPY=$(CIRCUITPY)

# Smaller builds can be forced for resource constrained chips (typically SAMD21s
# without external flash) by setting CIRCUITPY_FULL_BUILD=0. Avoid using this
# for merely incomplete ports, as it changes settings in other files.
CIRCUITPY_FULL_BUILD ?= 1
CFLAGS += -DCIRCUITPY_FULL_BUILD=$(CIRCUITPY_FULL_BUILD)

# By default, aggressively reduce the size of in-flash messages, at the cost of
# increased build time
CIRCUITPY_MESSAGE_COMPRESSION_LEVEL ?= 9

# Reduce the size of in-flash properties. Requires support in the .ld linker
# file, so not enabled by default.
CIRCUITPY_OPTIMIZE_PROPERTY_FLASH_SIZE ?= 0
CFLAGS += -DCIRCUITPY_OPTIMIZE_PROPERTY_FLASH_SIZE=$(CIRCUITPY_OPTIMIZE_PROPERTY_FLASH_SIZE)

# async/await language keyword support
MICROPY_PY_ASYNC_AWAIT ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DMICROPY_PY_ASYNC_AWAIT=$(MICROPY_PY_ASYNC_AWAIT)

# unused by CIRCUITPYTHON
MICROPY_ROM_TEXT_COMPRESSION = 0

# asyncio
# By default, include uasyncio if async/await are available.
MICROPY_PY_ASYNCIO ?= $(MICROPY_PY_ASYNC_AWAIT)
CFLAGS += -DMICROPY_PY_ASYNCIO=$(MICROPY_PY_ASYNCIO)

# asyncio normally needs select
MICROPY_PY_SELECT ?= $(MICROPY_PY_ASYNCIO)
CFLAGS += -DMICROPY_PY_SELECT=$(MICROPY_PY_SELECT)

# enable select.select if select is enabled.
MICROPY_PY_SELECT_SELECT ?= $(MICROPY_PY_SELECT)
CFLAGS += -DMICROPY_PY_SELECT_SELECT=$(MICROPY_PY_SELECT_SELECT)

CIRCUITPY_AESIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_AESIO=$(CIRCUITPY_AESIO)

# TODO: CIRCUITPY_ALARM will gradually be added to as many ports as possible
# so make this 1 or CIRCUITPY_FULL_BUILD eventually
CIRCUITPY_ALARM ?= 0
CFLAGS += -DCIRCUITPY_ALARM=$(CIRCUITPY_ALARM)

CIRCUITPY_ALARM_TOUCH ?= $(CIRCUITPY_ALARM)
CFLAGS += -DCIRCUITPY_ALARM_TOUCH=$(CIRCUITPY_ALARM_TOUCH)

CIRCUITPY_ANALOGBUFIO ?= 0
CFLAGS += -DCIRCUITPY_ANALOGBUFIO=$(CIRCUITPY_ANALOGBUFIO)

CIRCUITPY_ANALOGIO ?= 1
CFLAGS += -DCIRCUITPY_ANALOGIO=$(CIRCUITPY_ANALOGIO)

CIRCUITPY_ARRAY ?= 1
CFLAGS += -DCIRCUITPY_ARRAY=$(CIRCUITPY_ARRAY)

CIRCUITPY_ATEXIT ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_ATEXIT=$(CIRCUITPY_ATEXIT)

CIRCUITPY_AUDIOBUSIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_AUDIOBUSIO=$(CIRCUITPY_AUDIOBUSIO)

# Some boards have PDMIn but do not implement I2SOut.
CIRCUITPY_AUDIOBUSIO_I2SOUT ?= $(CIRCUITPY_AUDIOBUSIO)
CFLAGS += -DCIRCUITPY_AUDIOBUSIO_I2SOUT=$(CIRCUITPY_AUDIOBUSIO_I2SOUT)

# Likewise, some boards have I2SOut but do not implement PDMIn.
CIRCUITPY_AUDIOBUSIO_PDMIN ?= $(CIRCUITPY_AUDIOBUSIO)
CFLAGS += -DCIRCUITPY_AUDIOBUSIO_PDMIN=$(CIRCUITPY_AUDIOBUSIO_PDMIN)

CIRCUITPY_AUDIOIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_AUDIOIO=$(CIRCUITPY_AUDIOIO)

CIRCUITPY_AUDIOPWMIO ?= 0
CFLAGS += -DCIRCUITPY_AUDIOPWMIO=$(CIRCUITPY_AUDIOPWMIO)

CIRCUITPY_AUDIOCORE ?= $(call enable-if-any,$(CIRCUITPY_AUDIOPWMIO) $(CIRCUITPY_AUDIOIO) $(CIRCUITPY_AUDIOBUSIO))
CFLAGS += -DCIRCUITPY_AUDIOCORE=$(CIRCUITPY_AUDIOCORE)

CIRCUITPY_AUDIOMIXER ?= $(CIRCUITPY_AUDIOCORE)
CFLAGS += -DCIRCUITPY_AUDIOMIXER=$(CIRCUITPY_AUDIOMIXER)

ifndef CIRCUITPY_AUDIOCORE_DEBUG
CIRCUITPY_AUDIOCORE_DEBUG ?= 0
endif
CFLAGS += -DCIRCUITPY_AUDIOCORE_DEBUG=$(CIRCUITPY_AUDIOCORE_DEBUG)

CIRCUITPY_AUDIOMP3 ?= $(call enable-if-all,$(CIRCUITPY_FULL_BUILD) $(CIRCUITPY_AUDIOCORE))
CFLAGS += -DCIRCUITPY_AUDIOMP3=$(CIRCUITPY_AUDIOMP3)

CIRCUITPY_BINASCII ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_BINASCII=$(CIRCUITPY_BINASCII)

CIRCUITPY_BITBANG_APA102 ?= 0
CFLAGS += -DCIRCUITPY_BITBANG_APA102=$(CIRCUITPY_BITBANG_APA102)

CIRCUITPY_BITBANGIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_BITBANGIO=$(CIRCUITPY_BITBANGIO)

CIRCUITPY_BITOPS ?= 0
CFLAGS += -DCIRCUITPY_BITOPS=$(CIRCUITPY_BITOPS)

# _bleio can be supported on most any board via HCI
CIRCUITPY_BLEIO_HCI ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_BLEIO_HCI=$(CIRCUITPY_BLEIO_HCI)

# Explicitly enabled for boards that support _bleio.
CIRCUITPY_BLEIO ?= $(CIRCUITPY_BLEIO_HCI)
CFLAGS += -DCIRCUITPY_BLEIO=$(CIRCUITPY_BLEIO)

CIRCUITPY_BLE_FILE_SERVICE ?= 0
CFLAGS += -DCIRCUITPY_BLE_FILE_SERVICE=$(CIRCUITPY_BLE_FILE_SERVICE)

CIRCUITPY_BOARD ?= 1
CFLAGS += -DCIRCUITPY_BOARD=$(CIRCUITPY_BOARD)

CIRCUITPY_BUSDEVICE ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_BUSDEVICE=$(CIRCUITPY_BUSDEVICE)

CIRCUITPY_BUILTINS_POW3 ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_BUILTINS_POW3=$(CIRCUITPY_BUILTINS_POW3)

CIRCUITPY_BUSIO ?= 1
CFLAGS += -DCIRCUITPY_BUSIO=$(CIRCUITPY_BUSIO)

# Allow disabling of individual busio functionality.
# This should be used sparingly on specialized boards that can only implement parts of busio
# due to pin restrictions.
CIRCUITPY_BUSIO_I2C ?= $(CIRCUITPY_BUSIO)
CFLAGS += -DCIRCUITPY_BUSIO_I2C=$(CIRCUITPY_BUSIO_I2C)

CIRCUITPY_BUSIO_SPI ?= $(CIRCUITPY_BUSIO)
CFLAGS += -DCIRCUITPY_BUSIO_SPI=$(CIRCUITPY_BUSIO_SPI)

CIRCUITPY_BUSIO_UART ?= $(CIRCUITPY_BUSIO)
CFLAGS += -DCIRCUITPY_BUSIO_UART=$(CIRCUITPY_BUSIO_UART)

CIRCUITPY_CAMERA ?= 0
CFLAGS += -DCIRCUITPY_CAMERA=$(CIRCUITPY_CAMERA)

CIRCUITPY_CANIO ?= 0
CFLAGS += -DCIRCUITPY_CANIO=$(CIRCUITPY_CANIO)

CIRCUITPY_CODEOP ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_CODEOP=$(CIRCUITPY_CODEOP)

CIRCUITPY_COLLECTIONS ?= 1
CFLAGS += -DCIRCUITPY_COLLECTIONS=$(CIRCUITPY_COLLECTIONS)

CIRCUITPY_COMPUTED_GOTO_SAVE_SPACE ?= 0
CFLAGS += -DCIRCUITPY_COMPUTED_GOTO_SAVE_SPACE=$(CIRCUITPY_COMPUTED_GOTO_SAVE_SPACE)

CIRCUITPY_CYW43 ?= 0
CFLAGS += -DCIRCUITPY_CYW43=$(CIRCUITPY_CYW43)

CIRCUITPY_DIGITALIO ?= 1
CFLAGS += -DCIRCUITPY_DIGITALIO=$(CIRCUITPY_DIGITALIO)

CIRCUITPY_COPROC ?= 0
CFLAGS += -DCIRCUITPY_COPROC=$(CIRCUITPY_COPROC)

CIRCUITPY_COUNTIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_COUNTIO=$(CIRCUITPY_COUNTIO)

CIRCUITPY_DISPLAYIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_DISPLAYIO=$(CIRCUITPY_DISPLAYIO)

CIRCUITPY_BUSDISPLAY ?= $(CIRCUITPY_DISPLAYIO)
CFLAGS += -DCIRCUITPY_BUSDISPLAY=$(CIRCUITPY_BUSDISPLAY)

CIRCUITPY_FOURWIRE ?= $(CIRCUITPY_DISPLAYIO)
CFLAGS += -DCIRCUITPY_FOURWIRE=$(CIRCUITPY_FOURWIRE)

CIRCUITPY_EPAPERDISPLAY ?= $(CIRCUITPY_DISPLAYIO)
CFLAGS += -DCIRCUITPY_EPAPERDISPLAY=$(CIRCUITPY_EPAPERDISPLAY)

CIRCUITPY_I2CDISPLAYBUS ?= $(CIRCUITPY_DISPLAYIO)
CFLAGS += -DCIRCUITPY_I2CDISPLAYBUS=$(CIRCUITPY_I2CDISPLAYBUS)

ifeq ($(CIRCUITPY_DISPLAYIO),1)
CIRCUITPY_PARALLELDISPLAYBUS ?= $(CIRCUITPY_FULL_BUILD)
else
CIRCUITPY_PARALLELDISPLAYBUS = 0
endif
CFLAGS += -DCIRCUITPY_PARALLELDISPLAYBUS=$(CIRCUITPY_PARALLELDISPLAYBUS)

CIRCUITPY_DOTCLOCKFRAMEBUFFER ?= 0
CFLAGS += -DCIRCUITPY_DOTCLOCKFRAMEBUFFER=$(CIRCUITPY_DOTCLOCKFRAMEBUFFER)

# bitmapfilter, bitmaptools, and framebufferio rely on displayio and are not on small boards
CIRCUITPY_BITMAPFILTER ?= $(call enable-if-all,$(CIRCUITPY_FULL_BUILD) $(CIRCUITPY_DISPLAYIO))
CIRCUITPY_BITMAPTOOLS ?= $(call enable-if-all,$(CIRCUITPY_FULL_BUILD) $(CIRCUITPY_DISPLAYIO))
CIRCUITPY_FRAMEBUFFERIO ?= $(call enable-if-all,$(CIRCUITPY_FULL_BUILD) $(CIRCUITPY_DISPLAYIO))
CIRCUITPY_VECTORIO ?= $(CIRCUITPY_DISPLAYIO)
CFLAGS += -DCIRCUITPY_BITMAPFILTER=$(CIRCUITPY_BITMAPFILTER)
CFLAGS += -DCIRCUITPY_BITMAPTOOLS=$(CIRCUITPY_BITMAPTOOLS)
CFLAGS += -DCIRCUITPY_FRAMEBUFFERIO=$(CIRCUITPY_FRAMEBUFFERIO)
CFLAGS += -DCIRCUITPY_VECTORIO=$(CIRCUITPY_VECTORIO)

CIRCUITPY_DUALBANK ?= 0
CFLAGS += -DCIRCUITPY_DUALBANK=$(CIRCUITPY_DUALBANK)

# Enabled micropython.native decorator (experimental)
CIRCUITPY_ENABLE_MPY_NATIVE ?= 0
CFLAGS += -DCIRCUITPY_ENABLE_MPY_NATIVE=$(CIRCUITPY_ENABLE_MPY_NATIVE)

CIRCUITPY_OS_GETENV ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_OS_GETENV=$(CIRCUITPY_OS_GETENV)

CIRCUITPY_ERRNO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_ERRNO=$(CIRCUITPY_ERRNO)

# Espressif specific modules.
# Assume not an Espressif build.
CIRCUITPY_ESPIDF ?= 0
CFLAGS += -DCIRCUITPY_ESPIDF=$(CIRCUITPY_ESPIDF)

CIRCUITPY_ESPNOW ?= 0
CFLAGS += -DCIRCUITPY_ESPNOW=$(CIRCUITPY_ESPNOW)

CIRCUITPY_ESPULP ?= 0
CFLAGS += -DCIRCUITPY_ESPULP=$(CIRCUITPY_ESPULP)

CIRCUITPY_ESP_USB_SERIAL_JTAG ?= 0
CFLAGS += -DCIRCUITPY_ESP_USB_SERIAL_JTAG=$(CIRCUITPY_ESP_USB_SERIAL_JTAG)

CIRCUITPY_ESPCAMERA ?= 0
CFLAGS += -DCIRCUITPY_ESPCAMERA=$(CIRCUITPY_ESPCAMERA)

CIRCUITPY__EVE ?= 0
CFLAGS += -DCIRCUITPY__EVE=$(CIRCUITPY__EVE)

CIRCUITPY_FLOPPYIO ?= 0
CFLAGS += -DCIRCUITPY_FLOPPYIO=$(CIRCUITPY_FLOPPYIO)

CIRCUITPY_FREQUENCYIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_FREQUENCYIO=$(CIRCUITPY_FREQUENCYIO)

CIRCUITPY_FUTURE ?= 1
CFLAGS += -DCIRCUITPY_FUTURE=$(CIRCUITPY_FUTURE)

CIRCUITPY_GETPASS ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_GETPASS=$(CIRCUITPY_GETPASS)

CIRCUITPY_GIFIO ?= $(CIRCUITPY_DISPLAYIO)
CFLAGS += -DCIRCUITPY_GIFIO=$(CIRCUITPY_GIFIO)

CIRCUITPY_GNSS ?= 0
CFLAGS += -DCIRCUITPY_GNSS=$(CIRCUITPY_GNSS)

CIRCUITPY_HASHLIB ?= $(CIRCUITPY_WEB_WORKFLOW)
CFLAGS += -DCIRCUITPY_HASHLIB=$(CIRCUITPY_HASHLIB)

CIRCUITPY_HASHLIB_MBEDTLS ?= $(CIRCUITPY_HASHLIB)
CFLAGS += -DCIRCUITPY_HASHLIB_MBEDTLS=$(CIRCUITPY_HASHLIB_MBEDTLS)

# i.e., we need to include a subset of mbedtls only for hashlib's own needs
CIRCUITPY_HASHLIB_MBEDTLS_ONLY ?= $(call enable-if-all,$(CIRCUITPY_HASHLIB_MBEDTLS) $(call enable-if-not,$(CIRCUITPY_SSL)))
CFLAGS += -DCIRCUITPY_HASHLIB_MBEDTLS_ONLY=$(CIRCUITPY_HASHLIB_MBEDTLS_ONLY)

CIRCUITPY_I2CTARGET ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_I2CTARGET=$(CIRCUITPY_I2CTARGET)

CIRCUITPY_IMAGECAPTURE ?= 0
CFLAGS += -DCIRCUITPY_IMAGECAPTURE=$(CIRCUITPY_IMAGECAPTURE)

# io - needed by JSON support
CIRCUITPY_IO ?= $(CIRCUITPY_JSON)
CFLAGS += -DCIRCUITPY_IO=$(CIRCUITPY_IO)

# io.IOBase - enhances JPEG support
CIRCUITPY_IO_IOBASE ?= $(call enable-if-all,$(CIRCUITPY_IO) $(CIRCUITPY_JPEGIO))
CFLAGS += -DCIRCUITPY_IO_IOBASE=$(CIRCUITPY_IO_IOBASE)

CIRCUITPY_IPADDRESS ?= $(CIRCUITPY_WIFI)
CFLAGS += -DCIRCUITPY_IPADDRESS=$(CIRCUITPY_IPADDRESS)

CIRCUITPY_IS31FL3741 ?= 0
CFLAGS += -DCIRCUITPY_IS31FL3741=$(CIRCUITPY_IS31FL3741)

CIRCUITPY_JPEGIO ?= $(CIRCUITPY_BITMAPTOOLS)
CFLAGS += -DCIRCUITPY_JPEGIO=$(CIRCUITPY_JPEGIO)

CIRCUITPY_JSON ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_JSON=$(CIRCUITPY_JSON)

CIRCUITPY_KEYPAD ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_KEYPAD=$(CIRCUITPY_KEYPAD)

CIRCUITPY_KEYPAD_KEYS ?= $(CIRCUITPY_KEYPAD)
CFLAGS += -DCIRCUITPY_KEYPAD_KEYS=$(CIRCUITPY_KEYPAD_KEYS)

CIRCUITPY_KEYPAD_KEYMATRIX ?= $(CIRCUITPY_KEYPAD)
CFLAGS += -DCIRCUITPY_KEYPAD_KEYMATRIX=$(CIRCUITPY_KEYPAD_KEYMATRIX)

CIRCUITPY_KEYPAD_SHIFTREGISTERKEYS ?= $(CIRCUITPY_KEYPAD)
CFLAGS += -DCIRCUITPY_KEYPAD_SHIFTREGISTERKEYS=$(CIRCUITPY_KEYPAD_SHIFTREGISTERKEYS)

CIRCUITPY_KEYPAD_DEMUX ?= $(CIRCUITPY_KEYPAD)
CFLAGS += -DCIRCUITPY_KEYPAD_DEMUX=$(CIRCUITPY_KEYPAD_DEMUX)

CIRCUITPY_LOCALE ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_LOCALE=$(CIRCUITPY_LOCALE)

CIRCUITPY_MATH ?= 1
CFLAGS += -DCIRCUITPY_MATH=$(CIRCUITPY_MATH)

CIRCUITPY_MAX3421E ?= 0
CFLAGS += -DCIRCUITPY_MAX3421E=$(CIRCUITPY_MAX3421E)

CIRCUITPY_MEMORYMAP ?= 0
CFLAGS += -DCIRCUITPY_MEMORYMAP=$(CIRCUITPY_MEMORYMAP)

CIRCUITPY_MEMORYMONITOR ?= 0
CFLAGS += -DCIRCUITPY_MEMORYMONITOR=$(CIRCUITPY_MEMORYMONITOR)

CIRCUITPY_MICROCONTROLLER ?= 1
CFLAGS += -DCIRCUITPY_MICROCONTROLLER=$(CIRCUITPY_MICROCONTROLLER)

CIRCUITPY_MDNS ?= $(CIRCUITPY_WIFI)
CFLAGS += -DCIRCUITPY_MDNS=$(CIRCUITPY_MDNS)

CIRCUITPY_MSGPACK ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_MSGPACK=$(CIRCUITPY_MSGPACK)

CIRCUITPY_NEOPIXEL_WRITE ?= 1
CFLAGS += -DCIRCUITPY_NEOPIXEL_WRITE=$(CIRCUITPY_NEOPIXEL_WRITE)

CIRCUITPY_NVM ?= 1
CFLAGS += -DCIRCUITPY_NVM=$(CIRCUITPY_NVM)

CIRCUITPY_ONEWIREIO ?= $(CIRCUITPY_BUSIO)
CFLAGS += -DCIRCUITPY_ONEWIREIO=$(CIRCUITPY_ONEWIREIO)

CIRCUITPY_OPT_LOAD_ATTR_FAST_PATH ?= 1
CFLAGS += -DCIRCUITPY_OPT_LOAD_ATTR_FAST_PATH=$(CIRCUITPY_OPT_LOAD_ATTR_FAST_PATH)

CIRCUITPY_OPT_MAP_LOOKUP_CACHE ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_OPT_MAP_LOOKUP_CACHE=$(CIRCUITPY_OPT_MAP_LOOKUP_CACHE)

CIRCUITPY_OS ?= 1
CFLAGS += -DCIRCUITPY_OS=$(CIRCUITPY_OS)

CIRCUITPY_PEW ?= 0
CFLAGS += -DCIRCUITPY_PEW=$(CIRCUITPY_PEW)

# CIRCUITPY_PICODVI is handled in the raspberrypi tree.
# Only for RP2 chips. Assume not a raspberrypi build.
CIRCUITPY_PICODVI ?= 0
CFLAGS += -DCIRCUITPY_PICODVI=$(CIRCUITPY_PICODVI)

CIRCUITPY_PIXELBUF ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_PIXELBUF=$(CIRCUITPY_PIXELBUF)

CIRCUITPY_PIXELMAP ?= $(CIRCUITPY_PIXELBUF)
CFLAGS += -DCIRCUITPY_PIXELMAP=$(CIRCUITPY_PIXELMAP)

# Only for SAMD boards for the moment
CIRCUITPY_PS2IO ?= 0
CFLAGS += -DCIRCUITPY_PS2IO=$(CIRCUITPY_PS2IO)

CIRCUITPY_PULSEIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_PULSEIO=$(CIRCUITPY_PULSEIO)

# Allow disabling of pulseio.PulseOut
# This should be used sparingly on specialized boards that need PulseIin but
# don't have the space for PulseOut.
CIRCUITPY_PULSEIO_PULSEOUT ?= $(CIRCUITPY_PULSEIO)
CFLAGS += -DCIRCUITPY_PULSEIO_PULSEOUT=$(CIRCUITPY_PULSEIO_PULSEOUT)

 # TODO hakane: Maybe set this to 0 default, end explicitly to 1 in GCM4 only?
CIRCUITPY_ORGANIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_ORGANIO=$(CIRCUITPY_ORGANIO)

CIRCUITPY_PWMIO ?= 1
CFLAGS += -DCIRCUITPY_PWMIO=$(CIRCUITPY_PWMIO)

CIRCUITPY_QRIO ?= $(CIRCUITPY_IMAGECAPTURE)
CFLAGS += -DCIRCUITPY_QRIO=$(CIRCUITPY_QRIO)

CIRCUITPY_RAINBOWIO ?= 1
CFLAGS += -DCIRCUITPY_RAINBOWIO=$(CIRCUITPY_RAINBOWIO)

CIRCUITPY_RANDOM ?= 1
CFLAGS += -DCIRCUITPY_RANDOM=$(CIRCUITPY_RANDOM)

CIRCUITPY_RE ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_RE=$(CIRCUITPY_RE)

# Should busio.I2C() check for pullups?
# Some boards in combination with certain peripherals may not want this.
CIRCUITPY_REQUIRE_I2C_PULLUPS ?= 1
CFLAGS += -DCIRCUITPY_REQUIRE_I2C_PULLUPS=$(CIRCUITPY_REQUIRE_I2C_PULLUPS)

# Allow the use of strapping pins for i2c
CIRCUITPY_I2C_ALLOW_STRAPPING_PINS ?= 0
CFLAGS += -DCIRCUITPY_I2C_ALLOW_STRAPPING_PINS=$(CIRCUITPY_I2C_ALLOW_STRAPPING_PINS)

# CIRCUITPY_RP2PIO is handled in the raspberrypi tree.
# Only for rp2 chips.
# Assume not a rp2 build.
CIRCUITPY_RP2PIO ?= 0
CFLAGS += -DCIRCUITPY_RP2PIO=$(CIRCUITPY_RP2PIO)

CIRCUITPY_RGBMATRIX ?= 0
CFLAGS += -DCIRCUITPY_RGBMATRIX=$(CIRCUITPY_RGBMATRIX)

CIRCUITPY_ROTARYIO ?= 1
CFLAGS += -DCIRCUITPY_ROTARYIO=$(CIRCUITPY_ROTARYIO)

CIRCUITPY_ROTARYIO_SOFTENCODER ?= 0
CFLAGS += -DCIRCUITPY_ROTARYIO_SOFTENCODER=$(CIRCUITPY_ROTARYIO_SOFTENCODER)

CIRCUITPY_RTC ?= 1
CFLAGS += -DCIRCUITPY_RTC=$(CIRCUITPY_RTC)

# Enable support for safemode.py
CIRCUITPY_SAFEMODE_PY ?= 1
CFLAGS += -DCIRCUITPY_SAFEMODE_PY=$(CIRCUITPY_SAFEMODE_PY)

# CIRCUITPY_SAMD is handled in the atmel-samd tree.
# Only for SAMD chips.
# Assume not a SAMD build.
CIRCUITPY_SAMD ?= 0
CFLAGS += -DCIRCUITPY_SAMD=$(CIRCUITPY_SAMD)

CIRCUITPY_SDCARDIO ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_SDCARDIO=$(CIRCUITPY_SDCARDIO)

CIRCUITPY_SDIOIO ?= 0
CFLAGS += -DCIRCUITPY_SDIOIO=$(CIRCUITPY_SDIOIO)

CIRCUITPY_SERIAL_BLE ?= 0
CFLAGS += -DCIRCUITPY_SERIAL_BLE=$(CIRCUITPY_SERIAL_BLE)

CIRCUITPY_SETTABLE_PROCESSOR_FREQUENCY?= 0
CFLAGS += -DCIRCUITPY_SETTABLE_PROCESSOR_FREQUENCY=$(CIRCUITPY_SETTABLE_PROCESSOR_FREQUENCY)

CIRCUITPY_SHARPDISPLAY ?= $(CIRCUITPY_FRAMEBUFFERIO)
CFLAGS += -DCIRCUITPY_SHARPDISPLAY=$(CIRCUITPY_SHARPDISPLAY)

# Disable the safe mode blink at boot. Speeds up boot time, but makes it
# impossible to enter safe mode by pressing buttons on boot.
CIRCUITPY_SKIP_SAFE_MODE_WAIT ?= 0
CFLAGS += -DCIRCUITPY_SKIP_SAFE_MODE_WAIT=$(CIRCUITPY_SKIP_SAFE_MODE_WAIT)

CIRCUITPY_SOCKETPOOL ?= $(CIRCUITPY_WIFI)
CFLAGS += -DCIRCUITPY_SOCKETPOOL=$(CIRCUITPY_SOCKETPOOL)

CIRCUITPY_SSL ?= $(CIRCUITPY_WIFI)
CFLAGS += -DCIRCUITPY_SSL=$(CIRCUITPY_SSL)

CIRCUITPY_SSL_MBEDTLS ?= 0
CFLAGS += -DCIRCUITPY_SSL_MBEDTLS=$(CIRCUITPY_SSL_MBEDTLS)

# Currently always off.
CIRCUITPY_STAGE ?= 0
CFLAGS += -DCIRCUITPY_STAGE=$(CIRCUITPY_STAGE)

CIRCUITPY_STATUS_BAR ?= 1
CFLAGS += -DCIRCUITPY_STATUS_BAR=$(CIRCUITPY_STATUS_BAR)

CIRCUITPY_STORAGE ?= 1
CFLAGS += -DCIRCUITPY_STORAGE=$(CIRCUITPY_STORAGE)

CIRCUITPY_STORAGE_EXTEND ?= $(CIRCUITPY_DUALBANK)
CFLAGS += -DCIRCUITPY_STORAGE_EXTEND=$(CIRCUITPY_STORAGE_EXTEND)

CIRCUITPY_STRUCT ?= 1
CFLAGS += -DCIRCUITPY_STRUCT=$(CIRCUITPY_STRUCT)

CIRCUITPY_SUPERVISOR ?= 1
CFLAGS += -DCIRCUITPY_SUPERVISOR=$(CIRCUITPY_SUPERVISOR)

CIRCUITPY_SYNTHIO ?= $(CIRCUITPY_AUDIOCORE)
CFLAGS += -DCIRCUITPY_SYNTHIO=$(CIRCUITPY_SYNTHIO)

CIRCUITPY_SYNTHIO_MAX_CHANNELS ?= 2
CFLAGS += -DCIRCUITPY_SYNTHIO_MAX_CHANNELS=$(CIRCUITPY_SYNTHIO_MAX_CHANNELS)


CIRCUITPY_SYS ?= 1
CFLAGS += -DCIRCUITPY_SYS=$(CIRCUITPY_SYS)

CIRCUITPY_TERMINALIO ?= $(CIRCUITPY_DISPLAYIO)
CFLAGS += -DCIRCUITPY_TERMINALIO=$(CIRCUITPY_TERMINALIO)

CIRCUITPY_FONTIO ?= $(call enable-if-all,$(CIRCUITPY_DISPLAYIO) $(CIRCUITPY_TERMINALIO))
CFLAGS += -DCIRCUITPY_FONTIO=$(CIRCUITPY_FONTIO)

CIRCUITPY_TIME ?= 1
CFLAGS += -DCIRCUITPY_TIME=$(CIRCUITPY_TIME)

# touchio might be native or generic. See circuitpy_defns.mk.
CIRCUITPY_TOUCHIO_USE_NATIVE ?= 0
CFLAGS += -DCIRCUITPY_TOUCHIO_USE_NATIVE=$(CIRCUITPY_TOUCHIO_USE_NATIVE)

CIRCUITPY_TOUCHIO ?= 1
CFLAGS += -DCIRCUITPY_TOUCHIO=$(CIRCUITPY_TOUCHIO)

CIRCUITPY_TRACEBACK ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_TRACEBACK=$(CIRCUITPY_TRACEBACK)

# For debugging.
CIRCUITPY_UHEAP ?= 0
CFLAGS += -DCIRCUITPY_UHEAP=$(CIRCUITPY_UHEAP)

CIRCUITPY_USB_DEVICE ?= 1
CFLAGS += -DCIRCUITPY_USB_DEVICE=$(CIRCUITPY_USB_DEVICE)

ifneq ($(CIRCUITPY_USB_DEVICE),0)
ifndef USB_NUM_ENDPOINT_PAIRS
$(error "USB_NUM_ENDPOINT_PAIRS (number of USB endpoint pairs)must be defined")
endif
CFLAGS += -DUSB_NUM_ENDPOINT_PAIRS=$(USB_NUM_ENDPOINT_PAIRS)

# Compute these value once, so the shell command is not reinvoked many times.
USB_NUM_ENDPOINT_PAIRS_5_OR_GREATER := $(shell expr $(USB_NUM_ENDPOINT_PAIRS) '>=' 5)
USB_NUM_ENDPOINT_PAIRS_8_OR_GREATER := $(shell expr $(USB_NUM_ENDPOINT_PAIRS) '>=' 8)

# Some chips may not support the same number of IN or OUT endpoints as pairs.
# For instance, the ESP32-S2 only supports 5 IN endpoints at once, even though
# it has 7 endpoint pairs.
USB_NUM_IN_ENDPOINTS ?= $(USB_NUM_ENDPOINT_PAIRS)
CFLAGS += -DUSB_NUM_IN_ENDPOINTS=$(USB_NUM_IN_ENDPOINTS)

USB_NUM_OUT_ENDPOINTS ?= $(USB_NUM_ENDPOINT_PAIRS)
CFLAGS += -DUSB_NUM_OUT_ENDPOINTS=$(USB_NUM_OUT_ENDPOINTS)

CIRCUITPY_USB_IDENTIFICATION ?= $(CIRCUITPY_USB_DEVICE)
CFLAGS += -DCIRCUITPY_USB_IDENTIFICATION=$(CIRCUITPY_USB_IDENTIFICATION)

CIRCUITPY_USB_CDC ?= $(CIRCUITPY_USB_DEVICE)
CFLAGS += -DCIRCUITPY_USB_CDC=$(CIRCUITPY_USB_CDC)
CIRCUITPY_USB_CDC_CONSOLE_ENABLED_DEFAULT ?= 1
CFLAGS += -DCIRCUITPY_USB_CDC_CONSOLE_ENABLED_DEFAULT=$(CIRCUITPY_USB_CDC_CONSOLE_ENABLED_DEFAULT)
CIRCUITPY_USB_CDC_DATA_ENABLED_DEFAULT ?= 0
CFLAGS += -DCIRCUITPY_USB_CDC_DATA_ENABLED_DEFAULT=$(CIRCUITPY_USB_CDC_DATA_ENABLED_DEFAULT)

# HID is available by default, but is not turned on if there are fewer than 5 endpoints.
CIRCUITPY_USB_HID ?= $(CIRCUITPY_USB_DEVICE)
CFLAGS += -DCIRCUITPY_USB_HID=$(CIRCUITPY_USB_HID)
CIRCUITPY_USB_HID_ENABLED_DEFAULT ?= $(USB_NUM_ENDPOINT_PAIRS_5_OR_GREATER)
CFLAGS += -DCIRCUITPY_USB_HID_ENABLED_DEFAULT=$(CIRCUITPY_USB_HID_ENABLED_DEFAULT)

CIRCUITPY_USB_VIDEO ?= 0
CFLAGS += -DCIRCUITPY_USB_VIDEO=$(CIRCUITPY_USB_VIDEO)

CIRCUITPY_USB_HOST ?= 0
CFLAGS += -DCIRCUITPY_USB_HOST=$(CIRCUITPY_USB_HOST)

# MIDI is available by default, but is not turned on if there are fewer than 8 endpoints.
CIRCUITPY_USB_MIDI ?= $(CIRCUITPY_USB_DEVICE)
CFLAGS += -DCIRCUITPY_USB_MIDI=$(CIRCUITPY_USB_MIDI)
CIRCUITPY_USB_MIDI_ENABLED_DEFAULT ?= $(USB_NUM_ENDPOINT_PAIRS_8_OR_GREATER)
CFLAGS += -DCIRCUITPY_USB_MIDI_ENABLED_DEFAULT=$(CIRCUITPY_USB_MIDI_ENABLED_DEFAULT)

CIRCUITPY_USB_MSC ?= $(CIRCUITPY_USB_DEVICE)
CFLAGS += -DCIRCUITPY_USB_MSC=$(CIRCUITPY_USB_MSC)
CIRCUITPY_USB_MSC_ENABLED_DEFAULT ?= $(CIRCUITPY_USB_MSC)
CFLAGS += -DCIRCUITPY_USB_MSC_ENABLED_DEFAULT=$(CIRCUITPY_USB_MSC_ENABLED_DEFAULT)

# Defaulting this to OFF initially because it has only been tested on a
# limited number of platforms, and the other platforms do not have this
# setting in their mpconfigport.mk and/or mpconfigboard.mk files yet.
CIRCUITPY_USB_VENDOR ?= 0
CFLAGS += -DCIRCUITPY_USB_VENDOR=$(CIRCUITPY_USB_VENDOR)
endif


CIRCUITPY_PYUSB ?= $(call enable-if-any,$(CIRCUITPY_USB_HOST) $(CIRCUITPY_MAX3421E))
CFLAGS += -DCIRCUITPY_PYUSB=$(CIRCUITPY_PYUSB)

CIRCUITPY_TINYUSB_HOST ?= $(call enable-if-any,$(CIRCUITPY_USB_HOST) $(CIRCUITPY_MAX3421E))

CIRCUITPY_TINYUSB ?= $(call enable-if-any,$(CIRCUITPY_TINYUSB_HOST) $(CIRCUITPY_USB_DEVICE))
CFLAGS += -DCIRCUITPY_TINYUSB=$(CIRCUITPY_TINYUSB)

CIRCUITPY_USB_HOST ?= 0
CFLAGS += -DCIRCUITPY_USB_HOST=$(CIRCUITPY_USB_HOST)

CIRCUITPY_PYUSB ?= $(call enable-if-any,$(CIRCUITPY_USB_HOST) $(CIRCUITPY_MAX3421E))
CFLAGS += -DCIRCUITPY_PYUSB=$(CIRCUITPY_PYUSB)

CIRCUITPY_USB_KEYBOARD_WORKFLOW ?= $(call enable-if-any,$(CIRCUITPY_USB_HOST) $(CIRCUITPY_MAX3421E))
CFLAGS += -DCIRCUITPY_USB_KEYBOARD_WORKFLOW=$(CIRCUITPY_USB_KEYBOARD_WORKFLOW)

# For debugging.
CIRCUITPY_USTACK ?= 0
CFLAGS += -DCIRCUITPY_USTACK=$(CIRCUITPY_USTACK)

# for decompressing utilities
CIRCUITPY_ZLIB ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_ZLIB=$(CIRCUITPY_ZLIB)

# ulab numerics library
CIRCUITPY_ULAB ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_ULAB=$(CIRCUITPY_ULAB)

# whether to use -Os optimization on files in ulab
CIRCUITPY_ULAB_OPTIMIZE_SIZE ?= 0

# CIRCUITPY_VIDEOCORE is handled in the broadcom tree.
# Only for Broadcom chips.
# Assume not a Broadcom build.
CIRCUITPY_VIDEOCORE ?= 0
CFLAGS += -DCIRCUITPY_VIDEOCORE=$(CIRCUITPY_VIDEOCORE)

CIRCUITPY_WARNINGS ?= $(CIRCUITPY_FULL_BUILD)
CFLAGS += -DCIRCUITPY_WARNINGS=$(CIRCUITPY_WARNINGS)

# watchdog hardware support
CIRCUITPY_WATCHDOG ?= 0
CFLAGS += -DCIRCUITPY_WATCHDOG=$(CIRCUITPY_WATCHDOG)

CIRCUITPY_WIFI ?= 0
CFLAGS += -DCIRCUITPY_WIFI=$(CIRCUITPY_WIFI)

CIRCUITPY_WEB_WORKFLOW ?= $(CIRCUITPY_WIFI)
CFLAGS += -DCIRCUITPY_WEB_WORKFLOW=$(CIRCUITPY_WEB_WORKFLOW)

CIRCUITPY_WIFI_RADIO_SETTABLE_MAC_ADDRESS?= 1
CFLAGS += -DCIRCUITPY_WIFI_RADIO_SETTABLE_MAC_ADDRESS=$(CIRCUITPY_WIFI_RADIO_SETTABLE_MAC_ADDRESS)

# tinyusb port tailored configuration
CIRCUITPY_TUSB_MEM_ALIGN ?= 4
CFLAGS += -DCIRCUITPY_TUSB_MEM_ALIGN=$(CIRCUITPY_TUSB_MEM_ALIGN)

CIRCUITPY_TUSB_ATTR_USBRAM ?= ".bss.usbram"
CFLAGS += -DCIRCUITPY_TUSB_ATTR_USBRAM=$(CIRCUITPY_TUSB_ATTR_USBRAM)

# Output function trace information from the ARM ITM.
CIRCUITPY_SWO_TRACE ?= 0
CFLAGS += -DCIRCUITPY_SWO_TRACE=$(CIRCUITPY_SWO_TRACE)

# Define an equivalent for MICROPY_LONGINT_IMPL, to pass to $(MPY-TOOL) in py/mkrules.mk
# $(MPY-TOOL) needs to know what kind of longint to use (if any) to freeze long integers.
# This should correspond to the MICROPY_LONGINT_IMPL definition in mpconfigport.h.
#
# Also propagate longint choice from .mk to C. There's no easy string comparison
# in cpp conditionals, so we #define separate names for each.

ifeq ($(LONGINT_IMPL),NONE)
MPY_TOOL_LONGINT_IMPL = -mlongint-impl=none
CFLAGS += -DLONGINT_IMPL_NONE
else ifeq ($(LONGINT_IMPL),MPZ)
MPY_TOOL_LONGINT_IMPL = -mlongint-impl=mpz
CFLAGS += -DLONGINT_IMPL_MPZ
else ifeq ($(LONGINT_IMPL),LONGLONG)
MPY_TOOL_LONGINT_IMPL = -mlongint-impl=longlong
CFLAGS += -DLONGINT_IMPL_LONGLONG
else
$(error LONGINT_IMPL set to surprising value: "$(LONGINT_IMPL)")
endif
MPY_TOOL_FLAGS += $(MPY_TOOL_LONGINT_IMPL)

###
ifeq ($(LONGINT_IMPL),NONE)
else ifeq ($(LONGINT_IMPL),MPZ)
else ifeq ($(LONGINT_IMPL),LONGLONG)
else
$(error LONGINT_IMPL set to surprising value: "$(LONGINT_IMPL)")
endif

PREPROCESS_FROZEN_MODULES = PYTHONPATH=$(TOP)/tools/python-semver $(TOP)/tools/preprocess_frozen_modules.py
ifneq ($(FROZEN_MPY_DIRS),)
$(BUILD)/frozen_mpy: $(FROZEN_MPY_DIRS)
	$(ECHO) FREEZE $(FROZEN_MPY_DIRS)
	$(Q)$(MKDIR) -p $@
	$(Q)$(PREPROCESS_FROZEN_MODULES) -o $@ $(FROZEN_MPY_DIRS)

$(BUILD)/manifest.py: $(BUILD)/frozen_mpy | $(TOP)/py/circuitpy_mpconfig.mk mpconfigport.mk boards/$(BOARD)/mpconfigboard.mk
	$(ECHO) MKMANIFEST $(FROZEN_MPY_DIRS)
	$(Q)(cd $(BUILD)/frozen_mpy && find * -name \*.py -exec printf 'freeze_as_mpy("frozen_mpy", "%s")\n' {} \; )> $@.tmp && mv -f $@.tmp $@
FROZEN_MANIFEST=$(BUILD)/manifest.py
endif
