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

// sam11 software emulation of DEC PDP-11/40 KL11 Main TTY

#include "kl11.h"

#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "sam11.h"

#include <Arduino.h>

#if USE_11_45 && !STRICT_11_40
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
    case DEV_CONSOLE_TTY_IN_STATUS:
        return TKS;
    case DEV_CONSOLE_TTY_IN_DATA:
        if (TKS & 0x80)
        {
            TKS &= 0xff7e;
            return TKB;
        }
        return 0;
    case DEV_CONSOLE_TTY_OUT_STATUS:
        return TPS;
    case DEV_CONSOLE_TTY_OUT_DATA:
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
    case DEV_CONSOLE_TTY_IN_STATUS:
        if (v & (1 << 6))
        {
            TKS |= 1 << 6;
        }
        else
        {
            TKS &= ~(1 << 6);
        }
        break;
    case DEV_CONSOLE_TTY_OUT_STATUS:
        if (v & (1 << 6))
        {
            TPS |= 1 << 6;
        }
        else
        {
            TPS &= ~(1 << 6);
        }
        break;
    case DEV_CONSOLE_TTY_OUT_DATA:
        TPB = v & 0xff;
        TPS &= 0xff7f;
        count = 0;
        break;
    case DEV_CONSOLE_TTY_IN_DATA:
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
