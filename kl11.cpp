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

// sam11 software emulation of DEC PDP-11/40 KL11 Main TTY

#include "kl11.h"

#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "sam11.h"

#include <Arduino.h>

#if USE_11_45
#define procNS kb11
#else
#define procNS kd11
#endif

namespace kl11 {

uint16_t TKS;
uint16_t TKB;
uint16_t TPS;
uint16_t TPB;

void clearterminal()
{
    TKS = 0;
    TPS = 1 << 7;
    TKB = 0;
    TPB = 0;
}

static void addchar(char c)
{
    switch (c)
    {
    case 42:
        TKB = 4;
        break;
    case 19:
        TKB = 034;
        break;
        //case 46:
        //	TKB = 127;
    default:
        TKB = c;
        break;
    }
    TKS |= 0x80;
    if (TKS & (1 << 6))
    {
        procNS::interrupt(INTTTYIN, 4);
    }
}

uint8_t count;

void poll()
{
    if (Serial.available())
    {
        char c = Serial.read();
        if (FIRST_LF_BREAKS && (c == '\n' || c == '\r'))
            procNS::trapped = true;
        addchar(c & 0x7F);
    }

    if ((TPS & 0x80) == 0)
    {
        if (++count > 32)
        {
            Serial.write(TPB & 0x7f);  // the & 0x7f removes the parity bit, all characters should be 7-bit anyway.
            TPS |= 0x80;
            if (TPS & (1 << 6))
            {
                procNS::interrupt(INTTTYOUT, 4);
            }
        }
    }
}

uint16_t read16(uint32_t a)
{
    switch (a)
    {
    case 0777560:
        return TKS;
    case 0777562:
        if (TKS & 0x80)
        {
            TKS &= 0xff7e;
            return TKB;
        }
        return 0;
    case 0777564:
        return TPS;
    case 0777566:
        return 0;
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% kl11: read16 from invalid address"));  // " + ostr(a, 6))
            //panic();
        }
        return 0;
    }
}

void write16(uint32_t a, uint16_t v)
{
    switch (a)
    {
    case 0777560:
        if (v & (1 << 6))
        {
            TKS |= 1 << 6;
        }
        else
        {
            TKS &= ~(1 << 6);
        }
        break;
    case 0777564:
        if (v & (1 << 6))
        {
            TPS |= 1 << 6;
        }
        else
        {
            TPS &= ~(1 << 6);
        }
        break;
    case 0777566:
        TPB = v & 0xff;
        TPS &= 0xff7f;
        count = 0;
        break;
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% kl11: write16 to invalid address"));  // " + ostr(a, 6))
            //panic();
        }
    }
}

};  // namespace kl11
