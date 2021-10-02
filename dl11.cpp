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

// sam11 software emulation of DEC PDP-11/40 DL11 Main TTY

#include "dl11.h"

#include "pdp1140.h"

#if DL_TTYS

#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "sam11.h"

#include <Arduino.h>

#if USE_11_45
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
    serial[3] = &Serial4;

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
    case 42:
        TKB[t] = 4;
        break;
    case 19:
        TKB[t] = 034;
        break;
        //case 46:
        //	TKB = 127;
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
                    //panic();
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
            //panic();
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
                    //panic();
                }
            }
        }
    }
    else
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% dl11: write16 to invalid address"));  // " + ostr(a, 6))
            //panic();
        }
    }
}

};  // namespace dl11

#endif