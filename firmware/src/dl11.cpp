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

// sam11 software emulation of DEC PDP-11/40 DL11 Main TTY

#include "dl11.h"

#include "pdp1140.h"

#if DL_TTYS

#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "sam11.h"
#include "termopts.h"

#include <Arduino.h>

#if USE_11_45 && !STRICT_11_40
#define procNS kb11
#else
#define procNS kd11
#endif

namespace dl11 {

Uart* serial[4];

uint16_t TKS[4];
uint16_t TKB[4];
uint16_t TPS[4];
uint16_t TPB[4];

void begin(void)
{
    serial[0] = &Serial1;
    serial[1] = &Serial2;
    serial[2] = &Serial3;
    serial[3] = &Serial;

    for (int i = 0; i < 4; i++)
        serial[i]->begin(BAUD_DEFAULT);
}

void clearterminal(uint8_t t)
{
    if (t < 4)
    {
        TKS[t] = 0;
        TPS[t] = 1 << 7;
        TKB[t] = 0;
        TPB[t] = 0;
    }
}

static void addchar(char c, uint8_t t)
{
    switch (c)
    {
    default:
        TKB[t] = c;
        break;
    }
    TKS[t] |= 0x80;
    if (TKS[t] & (1 << 6))
    {
        procNS::interrupt(INTFLOAT + (010 * t) + 0, 4);
    }
}

uint8_t count[4];

void poll(uint8_t t)
{
    if (serial[t]->available())
    {
        char c = serial[t]->read();
        if (FIRST_LF_BREAKS && (c == '\n' || c == '\r'))
            procNS::trapped = true;
        addchar(c & 0x7F, t);
    }

    if ((TPS[t] & 0x80) == 0)
    {
        if (++count[t] > 32)
        {
            serial[t]->write(TPB[t] & 0x7f);  // the & 0x7f removes the parity bit, all characters should be 7-bit anyway.
            TPS[t] |= 0x80;
            if (TPS[t] & (1 << 6))
            {
                procNS::interrupt(INTFLOAT + (010 * t) + 4, 4);
            }
        }
    }
}

uint16_t read16(uint32_t a)
{
    if (a >= DEV_DL_1_TTY_IN_STATUS && a <= DEV_DL_16_OUT_DATA)
    {
        if (a >= DEV_DL_1_TTY_IN_STATUS && a <= DEV_DL_4_OUT_DATA)
        {
            uint8_t t = (a >> 3) & 070;

            switch (a & 0777707)
            {
            case DEV_DL_1_TTY_IN_STATUS:
                return TKS[t];
            case DEV_DL_1_TTY_IN_DATA:
                if (TKS[t] & 0x80)
                {
                    TKS[t] &= 0xff7e;
                    return TKB[t];
                }
                return 0;
            case DEV_DL_1_TTY_OUT_STATUS:
                return TPS[t];
            case DEV_DL_1_TTY_OUT_DATA:
                return 0;
            default:
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% dl11: read16 from invalid address"));  // " + ostr(a, 6))
                    // panic();
                }
                return 0;
            }
        }
    }
    else
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% dl11: read16 from invalid address"));  // " + ostr(a, 6))
            // panic();
        }
    }
}

void write16(uint32_t a, uint16_t v)
{
    if (a >= DEV_DL_1_TTY_IN_STATUS && a <= DEV_DL_16_OUT_DATA)
    {
        if (a >= DEV_DL_1_TTY_IN_STATUS && a <= DEV_DL_4_OUT_DATA)
        {
            uint8_t t = (a >> 3) & 070;

            switch (a & 0777707)
            {
            case DEV_DL_1_TTY_IN_STATUS:
                if (v & (1 << 6))
                {
                    TKS[t] |= 1 << 6;
                }
                else
                {
                    TKS[t] &= ~(1 << 6);
                }
                break;
            case DEV_DL_1_TTY_OUT_STATUS:
                if (v & (1 << 6))
                {
                    TPS[t] |= 1 << 6;
                }
                else
                {
                    TPS[t] &= ~(1 << 6);
                }
                break;
            case DEV_DL_1_TTY_OUT_DATA:
                TPB[t] = v & 0xff;
                TPS[t] &= 0xff7f;
                count = 0;
                break;
            case DEV_DL_1_TTY_IN_DATA:
                break;
            default:
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% dl11: write16 to invalid address"));  // " + ostr(a, 6))
                    // panic();
                }
            }
        }
    }
    else
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% dl11: write16 to invalid address"));  // " + ostr(a, 6))
            // panic();
        }
    }
}

};  // namespace dl11

#endif
