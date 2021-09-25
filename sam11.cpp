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

#if USE_11_45
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
        //delayMicroseconds(10);  // a touch of throttle... the processor is plenty fast enough, so we add this to mimic the slower pdp

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

        Serial.flush();
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
    Serial.println("%% Processor halted.");

#ifdef PIN_OUT_PROC_RUN
    digitalWrite(PIN_OUT_PROC_RUN, LED_OFF);
#endif

#ifdef PIN_OUT_PROC_STEP
    digitalWrite(PIN_OUT_PROC_STEP, LED_OFF);
#endif

    if (PRINTSIMLINES)
    {
        printstate();
    }
    Serial.flush();
    while (1)
        delay(1);
}
