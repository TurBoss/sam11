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

// sam11 software emulation of DEC DD11 UNIBUS Backplane

#include "dd11.h"

#include "kd11.h"
#include "kl11.h"
#include "kt11.h"
#include "kw11.h"
#include "ky11.h"
#include "ms11.h"
#include "rk11.h"
#include "sam11.h"
#include "xmem.h"

#include <SdFat.h>

namespace dd11 {

uint16_t read8(const uint32_t a)
{
    if (a < MAX_RAM_ADDRESS)
    {
        return ms11::read8(a);
    }
    if (a % 2 != 0)
    {
        return read16(a & ~1) >> 8;
    }
    return read16(a & ~1) & 0xFF;
}

void write8(const uint32_t a, const uint16_t v)
{
    if (a < MAX_RAM_ADDRESS)
    {
        ms11::write8(a, v);
        return;
    }
    if (a % 2 != 0)
    {
        write16(a & ~1, (read16(a) & 0xFF) | (v & 0xFF) << 8);
    }
    else
    {
        write16(a & ~1, (read16(a) & 0xFF00) | (v & 0xFF));
    }
}

void write16(uint32_t a, uint16_t v)
{
    if (a % 2 != 0)
    {
        Serial.print(F("%% dd11: write16 0"));
        Serial.print(v, OCT);
        Serial.print(F(" to odd address 0"));
        Serial.println(a, OCT);
        longjmp(trapbuf, INTBUS);
    }

    if (a < MAX_RAM_ADDRESS)
    {
        ms11::write16(a, v);
        return;
    }

    // if ((a & 0777770) == 0770000)
    // {
    //     kd11::R[a & 07] = v;
    //     return;
    // }

    switch (a)
    {
    case DEV_CPU_STAT:
        {
            int c14 = v >> 14;
            switch (c14)
            {
            case 0:
                kd11::switchmode(false);
                break;
            case 3:
                kd11::switchmode(true);
                break;
            default:
                if (PRINTSIMLINES)
                {
                    Serial.print(F("%% invalid PS (>>14) mode: "));
                    Serial.println(c14, OCT);
                }
                panic();
            }
            int c12 = (v >> 12) & 3;
            switch (c12)
            {
            case 0:
                kd11::prevuser = false;
                break;
            case 3:
                kd11::prevuser = true;
                break;
            default:
                if (PRINTSIMLINES)
                {
                    Serial.print(F("%% invalid PS (>>12 & 3) mode: "));
                    Serial.println(c12, OCT);
                }
                panic();
            }
            kd11::PS = v;
            return;
        }

    case DEV_STACK_LIM:
        kt11::SLR = v;  // probs wrong
        return;

    case DEV_LKS:
        kw11::LKS = v;
        return;

    case DEV_MMU_SR0:
        kt11::SR0 = v;
        return;

    case DEV_CONSOLE_DR:
        ky11::write16(a, v);
        return;

    case DEV_RK_DS:
    case DEV_RK_ER:
    case DEV_RK_CS:
    case DEV_RK_WC:
    case DEV_RK_BA:
    case DEV_RK_DA:
    case DEV_RK_DB:
        rk11::write16(a, v);
        return;

    default:
        break;
    }

    if ((a & 0777770) == 0777560)
    {
        kl11::write16(a, v);
        return;
    }
    if (((a & 0777600) == 0772200) || ((a & 0777600) == 0777600))
    {
        kt11::write16(a, v);
        return;
    }

    if (PRINTSIMLINES)
    {
        Serial.print(F("%% dd11: write to invalid address 0"));
        Serial.println(a, OCT);
    }

    longjmp(trapbuf, INTBUS);
}

uint16_t read16(uint32_t a)
{
    if (a % 2 != 0)
    {
        if (PRINTSIMLINES)
        {
            Serial.print(F("%% dd11: read16 from odd address 0"));
            Serial.println(a, OCT);
        }
        longjmp(trapbuf, INTBUS);
    }

    if (a < MAX_RAM_ADDRESS)  // if lower than the device memory, then this is just RAM
    {
        return ms11::read16(a);
    }

    // if ((a & 0777770) == 0770000)
    // {
    //     return kd11::R[a & 07];
    // }

    switch (a)  // Switch by address, and read from virtual device as appropriate
    {
    case DEV_CPU_STAT:
        return kd11::PS;

    case DEV_STACK_LIM:
        return kt11::SLR;  // probs wrong

    case DEV_LKS:
        return kw11::LKS;

    case DEV_MMU_SR0:
        return kt11::SR0;

    case DEV_MMU_SR2:
        return kt11::SR2;

    case DEV_CONSOLE_SR:
        return ky11::read16(a);

    case DEV_RK_DS:
    case DEV_RK_ER:
    case DEV_RK_CS:
    case DEV_RK_WC:
    case DEV_RK_BA:
    case DEV_RK_DA:
    case DEV_RK_DB:
        return rk11::read16(a);

    default:
        break;
    }

    if ((a & 0777770) == 0777560)
    {
        return kl11::read16(a);
    }

    if (((a & 0777600) == 0772200) || ((a & 0777600) == 0777600))
    {
        return kt11::read16(a);
    }
    if (PRINTSIMLINES)
    {
        Serial.print(F("%% dd11: read from invalid address 0"));
        Serial.println(a, OCT);
    }
    longjmp(trapbuf, INTBUS);
}

};  // namespace dd11
