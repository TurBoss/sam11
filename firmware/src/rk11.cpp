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

// sam11 software emulation of DEC PDP-11/40 RK11 RK Disk Controller
#include "rk11.h"

#include "dd11.h"
#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "platform.h"
#include "sam11.h"

#include <Arduino.h>
#include <SdFat.h>
#include <stdint.h>

#if USE_11_45 && !STRICT_11_40
#define procNS kb11
#else
#define procNS kd11
#endif

namespace rk11 {

uint32_t RKBA, RKDS, RKER, RKCS, RKWC, RKDA;
uint32_t drive, sector, surface, cylinder;

bool attached_drives[NUM_RK_DRIVES];
SdFile rkdata[NUM_RK_DRIVES];

uint16_t read16(uint32_t a)
{
    switch (a)
    {
    case DEV_RK_DS:  // Drive Status
        return RKDS;
    case DEV_RK_ER:  // Error Reg
        return RKER;
    case DEV_RK_CS:  // Control Status
        return RKCS | (RKBA & 0x30000) >> 12;
    case DEV_RK_WC:  // Word count
        return RKWC;
    case DEV_RK_BA:  // Bus Address
        return RKBA & 0xFFFF;
    case DEV_RK_DA:  // Disk Address
        RKDA = (sector) | (surface << 4) | (cylinder << 5) | (drive << 13);
        return RKDA;
    case DEV_RK_DB:  // Data Buffer
    case DEV_RK_MR:
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% rk11 read16 invalid read"));
        }
        //panic();
        return 0;
    }
}

static void rknotready()
{
#ifdef PIN_OUT_DISK_ACT
    digitalWrite(PIN_OUT_DISK_ACT, LED_ON);
#endif
    RKDS &= ~(1 << 6);
    RKCS &= ~(1 << 7);
}

static void rkready()
{
    RKDS |= 1 << 6;
    RKCS |= 1 << 7;
#ifdef PIN_OUT_DISK_ACT
    digitalWrite(PIN_OUT_DISK_ACT, LED_OFF);
#endif
}

void rkerror(uint16_t e)
{
    RKER |= e;
}

static void step()
{
again:
    RKER = 0;  // clear errors
    bool w;
    switch ((RKCS & 017) >> 1)
    {
    case 0:  // Controller reset
        reset();
        return;

    case 1:  // write
        w = true;
        break;

    case 2:  // read
        w = false;
        break;

    case 4:  // Seek
             // RKCS &= ~020000;
             // RKCS |= 0200;
             // procNS::interrupt(INTRK, 5);
             // return;

    case 6:  // Drive reset
             // RKER = 0;
             // RKDA &= 0160000;
             // RKCS &= ~020000;
             // RKCS |= 0200;
             // procNS::interrupt(INTRK, 5);
             // return;

    case 3:  // check
    case 5:  // read check
    case 7:  // write lock
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% unimplemented RK05 operation"));  //  %#o", ((r.RKCS & 017) >> 1)))
        }
        //panic();
        procNS::interrupt(INTRK, 5);
        return;
    }

    if (DEBUG_RK05)
    {
        if (PRINTSIMLINES)
        {
            Serial.print("%% rkstep: RKBA: ");
            Serial.print(RKBA, DEC);
            Serial.print(" RKWC: ");
            Serial.print(RKWC, DEC);
            Serial.print(" cylinder: ");
            Serial.print(cylinder, DEC);
            Serial.print(" sector: ");
            Serial.print(sector, DEC);
            Serial.print(" write: ");
            Serial.println(w ? "true" : "false");
        }
    }

    drive = (RKDA >> 13);
    if (drive > (NUM_RK_DRIVES - 1) || !attached_drives[drive])
    {
        rkerror(RKNXD);
    }
    if (cylinder > 0312)
    {
        rkerror(RKNXC);
    }
    if (sector > 013)
    {
        rkerror(RKNXS);
    }

    int32_t pos = (cylinder * 24 + surface * 12 + sector) * 512;
    if (!rkdata[drive].seekSet(pos))
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% rk11 step: failed to seek"));
        }
        panic();
    }

    uint16_t i;
    uint16_t val;
    for (i = 0; i < 256 && RKWC != 0; i++)
    {
        if (w)
        {
            val = dd11::read16(RKBA);
            rkdata[drive].write(val & 0xFF);
            rkdata[drive].write((val >> 8) & 0xFF);
        }
        else
        {
            int t = rkdata[drive].read();
            if (t == -1)
            {
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% rk11 step: failed to read (low)"));
                }
                panic();
            }
            val = t & 0xFF;

            t = rkdata[drive].read();
            if (t == -1)
            {
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% rk11 step: failed to read (high)"));
                }
                panic();
            }
            val += ((t & 0xFF) << 8);

            dd11::write16(RKBA, val);
        }
        RKBA += 2;
        RKWC = (RKWC + 1) & 0xFFFF;
    }
    sector++;
    if (sector > 013)
    {
        sector = 0;
        surface++;
        if (surface > 1)
        {
            surface = 0;
            cylinder++;
            if (cylinder > 0312)
            {
                rkerror(RKOVR);
            }
        }
    }
    if (RKWC == 0)
    {
        rkready();
        if (RKCS & (1 << 6))
        {
            procNS::interrupt(INTRK, 5);
        }
    }
    else
    {
        goto again;
    }
}

void write16(uint32_t a, uint16_t v)
{
    //printf("%% rkwrite: %06o\n",a);
    switch (a)
    {
    case DEV_RK_DS:  // Drive Status
        break;
    case DEV_RK_ER:  // Error Reg
        break;
    case DEV_RK_CS:  // Control Status
        RKBA = (RKBA & 0xFFFF) | ((v & 060) << 12);
        v &= 017517;  // writable bits
        RKCS &= ~017517;
        RKCS |= v & ~1;  // don't set GO bit
        if (v & 1)
        {
            switch ((RKCS & 017) >> 1)
            {
            case 0:
                reset();
                break;
            case 1:
            case 2:
                rknotready();
                step();
                break;
            default:
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% unimplemented RK05 operation"));  // %#o", ((r.RKCS & 017) >> 1)))
                }
                panic();
            }
        }
        break;
    case DEV_RK_WC:  // Word Count
        RKWC = v;
        break;
    case DEV_RK_BA:  // Bus Address
        RKBA = (RKBA & 0x30000) | (v);
        break;
    case DEV_RK_DA:  // Disk Address
        RKDA = v;
        drive = v >> 13;
        cylinder = (v >> 5) & 0377;
        surface = (v >> 4) & 1;
        sector = v & 15;
        break;
    case DEV_RK_DB:  // Data Buffer
    case DEV_RK_MR:
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% rk11 write16: invalid write"));
        }
        //panic();
    }
}

void reset()
{
    RKDS = (1 << 11) | (1 << 7) | (1 << 6);
    RKER = 0;
    RKCS = 1 << 7;
    RKWC = 0;
    RKBA = 0;
    RKDA = 0;
}

};  // namespace rk11
