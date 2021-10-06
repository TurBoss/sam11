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

// sam11 software emulation of DEC PDP-11/40 RL11 RL Disk Controller
#include "rl11.h"

#include "pdp1140.h"

#if RL_DRIVE

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

uint32_t RLBA, RLDA, RLMP, RLCS, RLBAE;
uint32_t drive, status, sectors, tracks, cylinders, m_addr;

SdFile rldata;

uint16_t read16(uint32_t a)
{
    switch (a)
    {
    case DEV_RL_CS:  // Control Status (OR in the bus extension)
        return RLCS | (RLBA & 0x30000) >> 12;
    case DEV_RL_MP:  // Multi Purpose
        return RLMP;
    case DEV_RL_BA:  // Bus Address
        return RLBA & 0xFFFF;
    case DEV_RL_DA:  // Disk Address
        return RLDA;
        if (m_addr >= 0)
            return m_addr;
    case DEV_RL_BAE:
        return (RLBA & 0x70000) >> 16;
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

    drive = (RLCS >> 8) & 3;
    RLCS &= ~1;

    bool w;
    switch ((RLCS & 017) >> 1)
    {
    case 0:  // no op
        return;
    case 1:  // write check
        return;
    case 2:  //get status
        if (RLMP & 0x8)
            RLCS &= 0x3F;
        RLMP = status | m_addr & 0100;
        return;
    case 3:  //seek
        if ((m_addr & 3) == 1)
        {
            if ((m_addr & 4))
            {
                RLDA = ((RLDA + (m_addr & 0xFF80)) & 0xFF80) | ((m_addr << 2) & 0x40);
            }
            else
            {
                RLDA = ((RLDA - (m_addr & 0xFF80)) & 0xFF80) | ((m_addr << 2) & 0x40);
            }
            m_addr = RLDA;
        }
        return;
    case 4:  // read header
        break;
    case 5:  // write data
        w = true;
        break;
    case 6:  // read data
    case 7:  // read no header check
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

    if ((m_addr >>> 6) >= tracks)
    {
        RLCS |= 0x9400;  // HNF
        break;
    }
    if ((m_addr & 0x3f) >= sectors)
    {
        RLCS |= 0x9400;  // HNF
        break;
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
    case DEV_RL_MP:  // Drive Status
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
                //panic();
            }
        }
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
    case DEV_RL_BAE:  // Data Buffer
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
