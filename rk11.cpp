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

namespace rk11 {

uint32_t RKBA, RKDS, RKER, RKCS, RKWC;
uint32_t drive, sector, surface, cylinder;

SdFile rkdata;

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
        return (sector) | (surface << 4) | (cylinder << 5) | (drive << 13);
    case DEV_RK_DB:  // Data Buffer
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% rk11::read16 invalid read"));
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
}

static void step()
{
again:
    bool w;
    switch ((RKCS & 017) >> 1)
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
            Serial.println(F("%% unimplemented RK05 operation"));  //  %#o", ((r.RKCS & 017) >> 1)))
        }
        panic();
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

    if (drive != 0)
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
    if (!rkdata.seekSet(pos))
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% rkstep: failed to seek"));
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
            rkdata.write(val & 0xFF);
            rkdata.write((val >> 8) & 0xFF);
        }
        else
        {
            int t = rkdata.read();
            if (t == -1)
            {
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% rkstep: failed to read (low)"));
                }
                panic();
            }
            val = t & 0xFF;

            t = rkdata.read();
            if (t == -1)
            {
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% rkstep: failed to read (high)"));
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
            kd11::interrupt(INTRK, 5);
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
        drive = v >> 13;
        cylinder = (v >> 5) & 0377;
        surface = (v >> 4) & 1;
        sector = v & 15;
        break;
    case DEV_RK_DB:  // Data Buffer
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% rkwrite16: invalid write"));
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
}

};  // namespace rk11
