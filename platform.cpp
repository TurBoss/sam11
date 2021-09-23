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

#include "platform.h"

#include "pdp1140.h"

#include <Arduino.h>

namespace platform {

// init the platform hardware
void begin()
{
#ifndef PIN_OUT_SD_CS
#if !USE_SDIOS
#error NO WAY TO USE SD CARD -> CANNOT COMPILE
#endif
#endif

#ifdef PIN_OUT_SD_CS
    pinMode(PIN_OUT_SD_CS, OUTPUT);  // SD card chip select
    digitalWrite(PIN_OUT_SD_CS, HIGH);
#endif

#ifdef PIN_OUT_DISK_ACT
    pinMode(PIN_OUT_DISK_ACT, OUTPUT);  // rk11 disk led
    digitalWrite(PIN_OUT_DISK_ACT, LED_OFF);
#endif

#ifdef PIN_OUT_PROC_STEP
    pinMode(PIN_OUT_PROC_STEP, OUTPUT);  // timing, high while CPU is stepping
    digitalWrite(PIN_OUT_PROC_STEP, LED_OFF);
#endif

#ifdef PIN_OUT_BUS_ACT
    pinMode(PIN_OUT_BUS_ACT, OUTPUT);  // Bus in use - not implemented
    digitalWrite(PIN_OUT_BUS_ACT, LED_OFF);
#endif

#ifdef PIN_OUT_MEM_ACT
    pinMode(PIN_OUT_MEM_ACT, OUTPUT);  // Memory in use - not implemented
    digitalWrite(PIN_OUT_MEM_ACT, LED_OFF);
#endif

#ifdef PIN_OUT_PROC_RUN
    pinMode(PIN_OUT_PROC_RUN, OUTPUT);  // Processor running
    digitalWrite(PIN_OUT_PROC_RUN, LED_OFF);
#endif

#if DISABLE_PIN_10
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
#endif

    return;
}

void writeAddr(uint32_t addr) { }     // write to address pins
void writeData(uint16_t data) { }     // write to data pins
void writeDispReg(uint16_t disp) { }  // write display register
uint16_t readSwitches()               // read processor front switches
{
    return INST_UNIX_SINGLEUSER;
}
};  // namespace platform
