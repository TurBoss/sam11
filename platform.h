/*
Modified BSD License

Copyright (c) 2021 Chloe Lunn

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "pdp1140.h"

#ifndef H_PLATFORM
#define H_PLATFORM

#define RAM_SPI      (0)  // Use SPI SRAM chips (not implemented)
#define RAM_INTERNAL (1)  // Use internal RAM addresses (must leave at least 8K for simulator) -> YOU MUST HAVE A LINKER SCRIPT THAT FORCES THE SOFTWARE TO BE LOW!
#define RAM_EXTENDED (2)  // Use extended, internal RAM (xmem library for AVRs)
#define RAM_PARALLEL (3)  // Use Parallel addr/data RAM chips (not implmented)
#define RAM_SWAPFILE (4)  // Use a file on the SD card as a swap file

#define LKS_LOW_ACC    (0)  // use elapsedMillis for the LKS tick
#define LKS_HIGH_ACC   (2)  // use elapsedMicros for the LKS tick
#define LKS_SHIFT_TICK (3)  // use loop counts as line clock

namespace platform {

void begin();
void writeAddr(uint32_t addr);
void writeData(uint16_t data);
void writeDispReg(uint16_t disp);
uint32_t readSwitches();
uint16_t readControlSwitches();

//-------------------------------------------------------------------------------------------------

// Arduino Mega and similar
#if defined(__AVR_ATmega2560__)

#define _printf printf

#define USE_SDIO false

#define ALLOW_DISASM    (true)
#define MAX_RAM_ADDRESS (0760000)  // 248KB

// See ATmega 2560 datasheet section 9.1
#define RAM_MODE     RAM_EXTENDED
#define RAM_PTR_ADDR (0x2200)  // where in memory is the RAM -> this is external to the main RAM addresses in AVRs (8K + 512B)

#define LED_ON  (LOW)
#define LED_OFF (HIGH)

#define PIN_OUT_SD_CS (4)
#define SD_SPEED_MHZ  (12)

#define PIN_OUT_DISK_ACT  (13)
#define PIN_OUT_PROC_STEP (18)
//#define PIN_OUT_PROC_RUN  (0)
//#define PIN_OUT_BUS_ACT   (0)

#define LKS_ACC LKS_SHIFT_TICK

//-------------------------------------------------------------------------------------------------

// Adafruit Grand Central M4 and similar
#elif defined(__SAMD51P20A__)

//#define BOOT_SCRIPT (true)  // Enable using the boot script to setup drives/etc.

#define _printf Serial.printf

#define ALLOW_DISASM    (true)     // allow disassembly (PDP-11) on crash/panic/state prints
#define MAX_RAM_ADDRESS (0760000)  // 248KB

#define RAM_MODE RAM_INTERNAL  // use the chip's onboard SRAM

#define LED_ON  (HIGH)
#define LED_OFF (LOW)

#define PIN_OUT_SD_CS SDCARD_SS_PIN  // already defined for us!
#define SD_SPEED_MHZ  (12)

#define PIN_OUT_DISK_ACT (13)
//#define PIN_OUT_MEM_ACT (13)
//#define PIN_OUT_PROC_STEP (13)
//#define PIN_OUT_PROC_RUN  (13)
//#define PIN_OUT_BUS_ACT   (13)
//#define PIN_OUT_USER_MODE (13)

#define LKS_ACC LKS_HIGH_ACC

//-------------------------------------------------------------------------------------------------

// Adafruit Feather M0 and similar -> warning, super duper slow...
#elif defined(__SAMD21G18A__)

#define _printf Serial.printf

#define ALLOW_DISASM    (false)    // allow disassembly (PDP-11) on crash/panic/state prints
#define MAX_RAM_ADDRESS (0760000)  // 248KB

#define RAM_MODE RAM_SWAPFILE  // use a swapfile as ram

#define LED_ON  (LOW)
#define LED_OFF (HIGH)

#define PIN_OUT_SD_CS (4)
#define SD_SPEED_MHZ  (12)

#define PIN_OUT_DISK_ACT  (6)
#define PIN_OUT_MEM_ACT   (1)
#define PIN_OUT_PROC_STEP (8)

#define DISABLE_PIN_10 (true)

//-------------------------------------------------------------------------------------------------

// Teensy 4.1 and similar -> wayyy fast
#elif defined(__IMXRT1062__)

#define _printf Serial.printf

#define USE_SDIO true  // use an SDIO interface for cards

#define ALLOW_DISASM    (false)    // allow disassembly (PDP-11) on crash/panic/state prints
#define MAX_RAM_ADDRESS (0760000)  // 248KB

#define RAM_MODE RAM_INTERNAL  // use the chip's onboard SRAM

#define LED_ON  (HIGH)
#define LED_OFF (LOW)

#define PIN_OUT_DISK_ACT (13)

#define LKS_ACC LKS_HIGH_ACC

//-------------------------------------------------------------------------------------------------

#endif

};  // namespace platform

#endif
