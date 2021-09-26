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

// sam11 software emulation of DEC PDP-11/40 RL11 RL Disk Controller
#include "rl11.h"

#if false

#include "dd11.h"
#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "platform.h"
#include "sam11.h"

#include <Arduino.h>
#include <SdFat.h>
#include <stdint.h>

#if USE_11_45
#define procNS kb11
#else
#define procNS kd11
#endif

namespace rl11 {

uint32_t RLBA, RLDA, RLMP, RLCS, RLWC;
uint32_t drive, sector, track, cylinder;

SdFile rldata;

uint16_t read16(uint32_t a)
{
    switch (a)
    {
    case DEV_RL_DS:  // Drive Status
        return RLDS;
    case DEV_RL_ER:  // Error Reg
        return RLER;
    case DEV_RL_CS:  // Control Status
        return RLCS | (RLBA & 0x30000) >> 12;
    case DEV_RL_WC:  // Word count
        return RLWC;
    case DEV_RL_BA:  // Bus Address
        return RLBA & 0xFFFF;
    case DEV_RL_DA:  // Disk Address
        return (sector) | (surface << 4) | (cylinder << 5) | (drive << 13);
    case DEV_RL_DB:  // Data Buffer
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% rl11::read16 invalid read"));
        }
        //panic();
        return 0;
    }
}

static void rlnotready()
{
#ifdef PIN_OUT_DISK_ACT
    digitalWrite(PIN_OUT_DISK_ACT, LED_ON);
#endif
    RLDS &= ~(1 << 6);
    RLCS &= ~(1 << 7);
}

static void rlready()
{
    RLDS |= 1 << 6;
    RLCS |= 1 << 7;
#ifdef PIN_OUT_DISK_ACT
    digitalWrite(PIN_OUT_DISK_ACT, LED_OFF);
#endif
}

void rlerror(uint16_t e)
{
}

static void step()
{
again:
    bool w;
    switch ((RLCS & 017) >> 1)
    {
    case 0:
        return;
    case 1:
        w = true;
        break;
    case 2:
        w = false;
        break;
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% unimplemented RL05 operation"));  //  %#o", ((r.RLCS & 017) >> 1)))
        }
        panic();
    }

    if (DEBUG_RL05)
    {
        if (PRINTSIMLINES)
        {
            Serial.print("%% rlstep: RLBA: ");
            Serial.print(RLBA, DEC);
            Serial.print(" RLWC: ");
            Serial.print(RLWC, DEC);
            Serial.print(" cylinder: ");
            Serial.print(cylinder, DEC);
            Serial.print(" sector: ");
            Serial.print(sector, DEC);
            Serial.print(" write: ");
            Serial.println(w ? "true" : "false");
        }
    }

    if (drive != 0)
    {
        rlerror(RLNXD);
    }
    if (cylinder > 0312)
    {
        rlerror(RLNXC);
    }
    if (sector > 013)
    {
        rlerror(RLNXS);
    }

    int32_t pos = (cylinder * 24 + surface * 12 + sector) * 512;
    if (!rldata.seekSet(pos))
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% rlstep: failed to seek"));
        }
        panic();
    }

    uint16_t i;
    uint16_t val;
    for (i = 0; i < 256 && RLWC != 0; i++)
    {
        if (w)
        {
            val = dd11::read16(RLBA);
            rldata.write(val & 0xFF);
            rldata.write((val >> 8) & 0xFF);
        }
        else
        {
            int t = rldata.read();
            if (t == -1)
            {
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% rlstep: failed to read (low)"));
                }
                panic();
            }
            val = t & 0xFF;

            t = rldata.read();
            if (t == -1)
            {
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% rlstep: failed to read (high)"));
                }
                panic();
            }
            val += ((t & 0xFF) << 8);

            dd11::write16(RLBA, val);
        }
        RLBA += 2;
        RLWC = (RLWC + 1) & 0xFFFF;
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
                rlerror(RLOVR);
            }
        }
    }
    if (RLWC == 0)
    {
        rlready();
        if (RLCS & (1 << 6))
        {
            procNS::interrupt(INTRL, 5);
        }
    }
    else
    {
        goto again;
    }
}

void write16(uint32_t a, uint16_t v)
{
    //printf("%% rlwrite: %06o\n",a);
    switch (a)
    {
    case DEV_RL_DS:  // Drive Status
        break;
    case DEV_RL_ER:  // Error Reg
        break;
    case DEV_RL_CS:  // Control Status
        RLBA = (RLBA & 0xFFFF) | ((v & 060) << 12);
        v &= 017517;  // writable bits
        RLCS &= ~017517;
        RLCS |= v & ~1;  // don't set GO bit
        if (v & 1)
        {
            switch ((RLCS & 017) >> 1)
            {
            case 0:
                reset();
                break;
            case 1:
            case 2:
                rlnotready();
                step();
                break;
            default:
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% unimplemented RL05 operation"));  // %#o", ((r.RLCS & 017) >> 1)))
                }
                panic();
            }
        }
        break;
    case DEV_RL_WC:  // Word Count
        RLWC = v;
        break;
    case DEV_RL_BA:  // Bus Address
        RLBA = (RLBA & 0x30000) | (v);
        break;
    case DEV_RL_DA:  // Disk Address
        drive = v >> 13;
        cylinder = (v >> 5) & 0377;
        surface = (v >> 4) & 1;
        sector = v & 15;
        break;
    case DEV_RL_DB:  // Data Buffer
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% rlwrite16: invalid write"));
        }
        //panic();
    }
}

void reset()
{
    RLCS = 0x81;
    RLBA = 0;
    RLDA = 0;
    RLMP = 0;
}

};  // namespace rl11

#endif