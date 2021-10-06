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

#include "sam11.h"

#include "dd11.h"
#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "kl11.h"
#include "kw11.h"
#include "ky11.h"
#include "ms11.h"
#include "pdp1140.h"
#include "platform.h"
#include "rk11.h"
#include "xmem.h"

#include <Arduino.h>
#include <SdFat.h>

#if USE_11_45 && !STRICT_11_40
#define procNS kb11
#else
#define procNS kd11
#endif

#if defined(__AVR_ATmega2560__)
int serialWrite(char c, FILE* f)
{
    Serial.write(c);
    return 0;
}
#endif

#if USE_SDIO
SdFatSdio sd;
#else
SdFat sd;
#endif

const char* disks[] =
  {
    "unixv6.dsk",
    "unixv5.dsk",
    "rsts.dsk",
    "xxdp.dsk",
    "rt11v3.dsk",
    "rt11v4.dsk",
    "rt11v5.dsk",
    "dos-11.dsk",
};

void setup(void)
{
    // init the board/platform
    platform::begin();

    // Start the UART
    Serial.begin(kl11::BAUD_DEFAULT);  // may open bootloader on some boards...

#if defined(__AVR_ATmega2560__)
    fdevopen(serialWrite, NULL);
#endif

    while (!Serial)
        ;

    // Serial.println("%% RK05 Boot List:");
    // Serial.println("%% ===================================================");
    // Serial.println("%%  0: UnixV6    - UnixV6 (default)");
    // Serial.println("%%  1: UnixV5    - UnixV5 (needs serial tty)");
    // Serial.println("%%  2: RSTS      - RSTS DEC OS (no boot)");
    // Serial.println("%%  3: XXDP      - XXDP DEC OS (no boot)");
    // Serial.println("%%  4: RT11 v3   - RT11 v3 DOS (no boot)");
    // Serial.println("%%  5: RT11 v4   - RT11 v4 DOS (no boot)");
    // Serial.println("%%  6: RT11 v5   - RT11 v5 DOS (no boot)");
    // Serial.println("%%  7: DOS-11v9  - DOS-11 v9 (irq fault)");

    // Serial.println("\r\n%% Type disk number followed by '.' to select image.");

    char disk;
    //     char c;
    //     char cnt;
    // reprompt:
    disk = 0;
    // cnt = 0;

    // Serial.print("%% disk=");
    // while (!Serial.available())
    //     delay(1);
    // c = 0;
    // while (1)
    // {
    //     c = Serial.read();
    //     if (isprint((c)))
    //     {
    //         if (c >= '0' && c <= '9')
    //         {
    //             Serial.print(c);
    //             disk += (c - '0') + (disk * 10 * cnt);
    //             cnt++;
    //         }
    //     }
    //     if (c == '.')
    //         break;
    //     c = 0;
    // }
    // Serial.println();

    // if (disk > 7)
    //     goto reprompt;
    if (PRINTSIMLINES)
    {
        Serial.print(F("%% Init disk "));
        Serial.print(disk, DEC);
        Serial.print(", ");
        Serial.println(disks[disk]);
    }
    // init the sd card
    if (!sd.begin(PIN_OUT_SD_CS, SD_SCK_MHZ(SD_SPEED_MHZ)))
        sd.initErrorHalt();

    // Initialise the RAM
    ms11::begin();

    // Load RK05 Disk 0 as Read/Write
    if (!rk11::rkdata.open(disks[disk], O_RDWR))
    {
        sd.errorHalt("%% opening disk for write failed");
    }

    ky11::reset();    // reset the front panel - sets the switches to INST_UNIX_SINGLEUSER (0173030)
    procNS::reset();  // reset the processor
    Serial.println(F("%% Ready\r\n"));
}

static void loop0()
{
    while (1)
    {
        // Check for interruptd
        if ((itab[0].vec) && (itab[0].pri >= ((procNS::PS >> 5) & 7)))
        {
            procNS::handleinterrupt();
            return;  // reset the loop to reset interrupt
        }

#ifdef PIN_OUT_PROC_STEP
        digitalWrite(PIN_OUT_PROC_STEP, LED_ON);
#endif

        procNS::step();  // step the instructions

#ifdef PIN_OUT_PROC_STEP
        digitalWrite(PIN_OUT_PROC_STEP, LED_OFF);
#endif

        kw11::tick();  // tick the clock

        kl11::poll();  // check the terminal
    }
}

jmp_buf trapbuf;

void loop()
{
    // reset interrupts/trap vectors
    uint16_t vec = setjmp(trapbuf);
    if (vec)
    {
        procNS::trapat(vec);
    }
    loop0();  // restart step loop
}

void panic()  // aka what it does when halted
{
#ifdef PIN_OUT_PROC_RUN
    digitalWrite(PIN_OUT_PROC_RUN, LED_OFF);
#endif

#ifdef PIN_OUT_PROC_STEP
    digitalWrite(PIN_OUT_PROC_STEP, LED_OFF);
#endif

    rk11::rkdata.close();  // missing from original avr software... I corrupted a few UNIX disks working this one out!
    Serial.println("%% Processor halted.");
    if (PRINTSIMLINES)
    {
        printstate();
    }
    Serial.flush();
    while (1)
        delay(1);
}
