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

// sam11 software emulation of DEC DD11 UNIBUS Backplane

#include "dd11.h"

#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "kl11.h"
#include "kt11.h"
#include "kw11.h"
#include "ky11.h"
#include "lp11.h"
#include "ms11.h"
#include "rk11.h"
#include "sam11.h"
#include "xmem.h"

#if USE_11_45 && !STRICT_11_40
#define procNS kb11
#else
#define procNS kd11
#endif

#if KY_PANEL
#define readReturn res =
#else
#define readReturn return
#endif

#include <SdFat.h>

namespace dd11 {

uint16_t read8(const uint32_t a)
{
#if !KY_PANEL
    // If the KY_PANEL is enabled, route everything as words
    if (a < MAX_RAM_ADDRESS)
    {
        return ms11::read8(a);
    }
#endif
    if (a % 2 != 0)
    {
        return read16(a & ~1) >> 8;
    }
    return read16(a & ~1) & 0xFF;
}

void write8(const uint32_t a, const uint16_t v)
{
#if !KY_PANEL
    // If the KY_PANEL is enabled, route everything as words
    if (a < MAX_RAM_ADDRESS)
    {
        ms11::write8(a, v);
        return;
    }
#endif
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
        if (PRINTSIMLINES)
        {
            Serial.print(F("%% dd11: write16 0"));
            Serial.print(v, OCT);
            Serial.print(F(" to odd address 0"));
            Serial.println(a, OCT);
        }
        longjmp(trapbuf, INTBUS);
    }

#if KY_PANEL
    ky11::write16(a, v);
#endif

    if (a < MAX_RAM_ADDRESS)
    {
        ms11::write16(a, v);
        return;
    }

    switch (a)
    {
    case DEV_CPU_STAT:
        {
            int c14 = v >> 14;
            switch (c14)
            {
            case 0:
                procNS::switchmode(0);  //Kernel
                break;
#if !STRICT_11_40
            case 1:
                procNS::switchmode(1);  // Super
                break;
#endif
            case 3:
                procNS::switchmode(3);  // User
                break;
            default:
                if (PRINTSIMLINES)
                {
                    Serial.print(F("%% invalid current user mode: "));
                    Serial.println(c14, OCT);
                }
                panic();
            }
            int c12 = (v >> 12) & 3;
            switch (c12)
            {
            case 0:
                procNS::prevuser = 0;  // Kernel
                break;
#if !STRICT_11_40
            case 1:
                procNS::prevuser = 1;  // Super
                break;
#endif
            case 3:
                procNS::prevuser = 3;  // User
                break;
            default:
                if (PRINTSIMLINES)
                {
                    Serial.print(F("%% invalid previous user mode: "));
                    Serial.println(c12, OCT);
                }
                panic();
            }
            procNS::PS = v;
        }
        return;

    case DEV_CPU_KER_PC:
        procNS::curPC;
        return;

#if !STRICT_11_40
    case DEV_CPU_SUP_SP:
        {
            if (procNS::curuser == 1)
                procNS::R[6] = v;
            else
                procNS::SSP = v;
        }
        return;
#endif

    case DEV_CPU_KER_SP:
        {
            if (procNS::curuser == 0)
                procNS::R[6] = v;
            else
                procNS::KSP = v;
        }
        return;

    case DEV_CPU_USR_SP:
        {
            if (procNS::curuser == 3)
                procNS::R[6] = v;
            else
                procNS::USP = v;
        }
        return;

#if !STRICT_11_40
    case DEV_STACK_LIM:
        kt11::SLR = v | 0377;  // probs wrong
        return;
#endif

    case DEV_KW_LKS:
        kw11::LKS = v;
        return;

    case DEV_MMU_SR0:
        kt11::SR0 = v;
        return;

#if !STRICT_11_40
    case DEV_MMU_SR1:
        kt11::SR1 = v;
        return;
#endif

    case DEV_MMU_SR2:
        kt11::SR2 = v;
        return;

#if !STRICT_11_40
    case DEV_MMU_SR3:
        kt11::SR3 = v;
        return;
#endif

#if !KY_PANEL
        // If the KY_PANEL is enabled, then it already got this info, so don't sent it again
    case DEV_CONSOLE_DR:
        ky11::write16(a, v);
        return;
#endif

#if USE_LP
    case DEV_LP_DATA:
    case DEV_LP_STATUS:
        lp11::write16(a, v);
        return;
#endif

    case DEV_RK_DS:
    case DEV_RK_ER:
    case DEV_RK_CS:
    case DEV_RK_WC:
    case DEV_RK_BA:
    case DEV_RK_DA:
    case DEV_RK_DB:
    case DEV_RK_MR:
        rk11::write16(a, v);
        return;

    case DEV_CONSOLE_TTY_OUT_DATA:
    case DEV_CONSOLE_TTY_OUT_STATUS:
    case DEV_CONSOLE_TTY_IN_DATA:
    case DEV_CONSOLE_TTY_IN_STATUS:
        kl11::write16(a, v);
        return;

    default:
        break;
    }

#if !STRICT_11_40
    // don't use switch/case for this because there would be like 112 lines of "case DEV_USR_DAT_PAR_R7:"
    if (((a & 0777700) == DEV_SUP_INS_PDR_R0) || ((a & 0777700) == DEV_USR_INS_PDR_R0) || ((a & 0777700) == DEV_KER_INS_PDR_R0))
    {
        kt11::write16(a, v);
        return;
    }
#else
    // don't use switch/case for this because there would be like 112 lines of "case DEV_USR_DAT_PAR_R7:"
    if (((a & 0777700) == DEV_KER_INS_PDR_R0) || ((a & 0777700) == DEV_USR_INS_PDR_R0))
    {
        kt11::write16(a, v);
        return;
    }
#endif

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

#if KY_PANEL
    int res = 0;
#endif

    if (a < MAX_RAM_ADDRESS)  // if lower than the device memory, then this is just RAM
    {
        readReturn ms11::read16(a);
    }

    switch (a)  // Switch by address, and read from virtual device as appropriate
    {
    case DEV_CPU_STAT:
        readReturn procNS::PS;
        break;

    case DEV_CPU_KER_PC:
        readReturn procNS::curPC;
        break;

#if !STRICT_11_40
    case DEV_CPU_SUP_SP:
        {
            if (procNS::curuser == 1)
                readReturn procNS::R[6];
            else
                readReturn procNS::SSP;
        }
        break;
#endif

    case DEV_CPU_KER_SP:
        {
            if (procNS::curuser == 0)
                readReturn procNS::R[6];
            else
                readReturn procNS::KSP;
        }
        break;

    case DEV_CPU_USR_SP:
        {
            if (procNS::curuser == 3)
                readReturn procNS::R[6];
            else
                readReturn procNS::USP;
        }
        break;

#if !STRICT_11_40
    case DEV_STACK_LIM:
        readReturn kt11::SLR & 0177400;  // probs wrong
        break;
#endif

    case DEV_KW_LKS:
        readReturn kw11::LKS;
        break;

    case DEV_MMU_SR0:
        readReturn kt11::SR0;
        break;

#if !STRICT_11_40
    case DEV_MMU_SR1:
        readReturn kt11::SR1;
        break;
#endif

    case DEV_MMU_SR2:
        readReturn kt11::SR2;
        break;

#if !STRICT_11_40
    case DEV_MMU_SR3:
        readReturn kt11::SR3;
        break;
#endif

    case DEV_CONSOLE_SR:
        readReturn ky11::read16(a);
        break;

#if USE_LP
    case DEV_LP_DATA:
    case DEV_LP_STATUS:
        readReturn lp11::read16(a);
        break;
#endif

    case DEV_RK_DS:
    case DEV_RK_ER:
    case DEV_RK_CS:
    case DEV_RK_WC:
    case DEV_RK_BA:
    case DEV_RK_DA:
    case DEV_RK_DB:
    case DEV_RK_MR:
        readReturn rk11::read16(a);
        break;

    case DEV_CONSOLE_TTY_OUT_DATA:
    case DEV_CONSOLE_TTY_OUT_STATUS:
    case DEV_CONSOLE_TTY_IN_DATA:
    case DEV_CONSOLE_TTY_IN_STATUS:
        readReturn kl11::read16(a);
        break;

    default:
        break;
    }

#if !STRICT_11_40
    // don't use switch/case for this because there would be like 112 lines of "case DEV_USR_DAT_PAR_R7:"
    if (((a & 0777700) == DEV_SUP_INS_PDR_R0) || ((a & 0777700) == DEV_USR_INS_PDR_R0) || ((a & 0777700) == DEV_KER_INS_PDR_R0))
    {
        readReturn kt11::read16(a);
    }
#else
    // don't use switch/case for this because there would be like 112 lines of "case DEV_USR_DAT_PAR_R7:"
    if (((a & 0777700) == DEV_KER_INS_PDR_R0) || ((a & 0777700) == DEV_USR_INS_PDR_R0))
    {
        readReturn kt11::read16(a);
    }
#endif

#if KY_PANEL
    // If the panel is enabled, bus access gets written to the front panel, EXCEPT the switch registers, because that would be weird, instead we just do the address there
    if (a != DEV_CONSOLE_SR)
        ky11::write16(a, res);
    else
        ky11::write16(a, 0);
    return res;
#endif

    if (PRINTSIMLINES)
    {
        Serial.print(F("%% dd11: read from invalid address 0"));
        Serial.println(a, OCT);
    }
    longjmp(trapbuf, INTBUS);
}

};  // namespace dd11
