/*
MIT License

Copyright (c) 2021 Chloe Lunn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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
uint16_t readSwitches();

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

#define _printf Serial.printf

#define USE_SDIO false  // use an SDIO interface for cards

#define ALLOW_DISASM    (false)  // allow disassembly (PDP-11) on crash/panic/state prints
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

#define USE_SDIO false  // use an SDIO interface for cards

#define ALLOW_DISASM    (false)  // allow disassembly (PDP-11) on crash/panic/state prints
#define MAX_RAM_ADDRESS (0760000)  // 248KB

#define RAM_MODE RAM_SWAPFILE  // use a swapfile as ram

#define LED_ON  (LOW)
#define LED_OFF (HIGH)

#define PIN_OUT_SD_CS (4)
#define SD_SPEED_MHZ  (12)

#define PIN_OUT_DISK_ACT  (6)
#define PIN_OUT_MEM_ACT   (1)
#define PIN_OUT_PROC_STEP (8)
//#define PIN_OUT_PROC_RUN  (13)
//#define PIN_OUT_BUS_ACT   (13)
//#define PIN_OUT_USER_MODE (13)
#define DISABLE_PIN_10    (true)

//-------------------------------------------------------------------------------------------------

#endif

};  // namespace platform

#endif
