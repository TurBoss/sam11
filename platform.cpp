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

#include "platform.h"

#include "pdp1140.h"

#include <Arduino.h>

namespace platform {

// init the platform hardware
void begin()
{
    // no SD control options
#ifndef PIN_OUT_SD_CS
#if !USE_SDIO
#error NO WAY TO USE SD CARD -> CANNOT COMPILE
#endif
#endif

#if defined(PIN_OUT_SD_CS) && !USE_SDIO  // If SPI SD, then init the CS line
    pinMode(PIN_OUT_SD_CS, OUTPUT);      // SD card chip select
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
    return 0;  //INST_UNIX_SINGLEUSER;
}
};  // namespace platform
