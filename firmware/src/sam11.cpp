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
#include "ini.h"
#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "kl11.h"
#include "kw11.h"
#include "ky11.h"
#include "lp11.h"
#include "ms11.h"
#include "pdp1140.h"
#include "platform.h"
#include "rk11.h"
#include "termopts.h"
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

#if USE_SDIO && !defined(__IMXRT1062__)  // If SDIO and not a Teensy 4/4.1
SdFatSdio sd;
#else  // SPI or Teensy
SdFat sd;
#endif

#if USE_11_45 && !STRICT_11_40
const char* users_str[4] = {
  "kernel",
  "superviser",
  "illegal",
  "user"};
const char users_char[4] = {
  'K',
  'S',
  'X',
  'U'};
#else
const char* users_str[4] = {
  "kernel",
  "illegal",
  "illegal",
  "user"};
const char users_char[4] = {
  'K',
  'X',
  'X',
  'U'};
#endif

void setup(void)
{
    // init the board/platform
    platform::begin();

    // Start the UART
    Serial.begin(kl11::BAUD_DEFAULT);  // may open bootloader on some boards if the baud is 1200...

#if defined(__AVR_ATmega2560__)
    fdevopen(serialWrite, NULL);
#endif

    while (!Serial)
        ;

        // init the sd card
#if !USE_SDIO && !defined(__IMXRT1062__)  // SPI
    if (!sd.begin(PIN_OUT_SD_CS, SD_SCK_MHZ(SD_SPEED_MHZ)))
        sd.initErrorHalt();
#elif USE_SDIO && defined(__IMXRT1062__)  // SDIO and Teensy
    if (!sd.begin(SdioConfig(FIFO_SDIO)))
        sd.initErrorHalt();
#else                                     // Normal SDIO
    if (!sd.begin())
        sd.initErrorHalt();
#endif

    // Initialise the RAM
    ms11::begin();

#if BOOT_SCRIPT
    // Try to open the boot script, and execute if you can
    File boot_script;
    if (boot_script.open("boot.ini", O_READ))
    {
        String line;
        int l = 0;
        while (boot_script.available())
        {
            line = boot_script.readStringUntil('\n');
            Serial.print("%%> ");
            Serial.println(line);
            ini::setup_fr_line((char*)line.c_str());
            _printf("%%%% Parsed line %i\r\n", l++);
        }
        boot_script.close();
    }
#else

#if NUM_RK_DRIVES >= 1
    // Load RK05 Disk 0 as Read/Write
    if (!rk11::rkdata[0].open("unixv6.dsk", O_RDWR))
    {
        sd.errorHalt("%% opening RK disk 0 for write failed");
        rk11::attached_drives[0] = false;
    }
    else
        rk11::attached_drives[0] = true;
#endif

#if NUM_RK_DRIVES >= 2
    // Load RK05 Disk 1 as Read/Write
    if (!rk11::rkdata[1].open("rk1.dsk", O_RDWR))
    {
        if (PRINTSIMLINES)
            Serial.println("%% opening RK disk 1 for write failed");
        rk11::attached_drives[1] = false;
    }
    else
        rk11::attached_drives[1] = true;
#endif

#if NUM_RK_DRIVES >= 3
    // Load RK05 Disk 2 as Read/Write
    if (!rk11::rkdata[2].open("rk2.dsk", O_RDWR))
    {
        if (PRINTSIMLINES)
            Serial.println("%% opening RK disk 2 for write failed");
        rk11::attached_drives[2] = false;
    }
    else
        rk11::attached_drives[2] = true;
#endif

#if NUM_RK_DRIVES >= 4
    // Load RK05 Disk 3 as Read/Write
    if (!rk11::rkdata[3].open("csd.dsk", O_RDWR))
    {
        if (PRINTSIMLINES)
            Serial.println("%% opening RK disk 3 for write failed");
        rk11::attached_drives[3] = false;
    }
    else
        rk11::attached_drives[3] = true;
#endif

#endif

    ky11::reset();    // reset the front panel - sets the switches to INST_UNIX_SINGLEUSER (0173030)
    procNS::reset();  // reset the processor
    Serial.println(F("%% Ready\r\n"));
    Serial.write(7);  // write out a bell.

#if FLUSH_SERIAL_AT_READY
    delay(100);
    Serial.flush();
    delay(100);
    while (Serial.available())
    {
        char dump = Serial.read();
    }
#endif
}

static void loop0()
{
    while (1)
    {
        // Check for interrupts
        if ((itab[0].vec) && (itab[0].pri >= ((procNS::PS >> 5) & 7)))
        {
            procNS::handleinterrupt();
            return;  // reset the loop to reset interrupt
        }

#if KY_PANEL
        ky11::step();
#endif

#if USE_LP
        lp11::poll();
#endif

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

    for (int i = 0; i < NUM_RK_DRIVES; i++)
        if (rk11::attached_drives[i])
            rk11::rkdata[i].close();  // I corrupted a few UNIX disks working this one out! Whoops!

#ifdef PIN_OUT_DISK_ACT
    digitalWrite(PIN_OUT_DISK_ACT, LED_OFF);
#endif

    Serial.println("%% Processor halted.");
    if (PRINTSIMLINES)
    {
        printstate();
    }
    Serial.write(7);  // write out a bell
    Serial.flush();
    while (1)
        delay(1);
}
