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

// sam11 software emulation of DEC PDP-11/40 KT11 Memory Management Unit (MMU)

#include "kt11.h"

#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "platform.h"
#include "sam11.h"

#include <Arduino.h>

#if USE_11_45 && !STRICT_11_40
#define procNS kb11
#else
#define procNS kd11
#endif

namespace kt11 {

uint16_t SLR;

struct page {
    uint16_t par;  // Page address register
    uint16_t pdr;  // Page descriptor register
    uint32_t addr()
    {
        return par & 07777;  // only bits 11-0 are valid
    }
    uint16_t len()
    {
        return ((pdr >> 8) & 0x7F);  // page length field - bits 14-8
    }
    bool read()
    {
        return (pdr & 2) == 2;
    }
    bool write()
    {
        return (pdr & 6) == 6;
    }
    bool ed()  // expansion director, 0=up, 1=down
    {
        return (pdr & 8) == 8;
    }
};

page instr_pages[4][8];  //0 = kern, 1 = super, 2 = illegal, 3 = user
page data_pages[4][8];   //0 = kern, 1 = super, 2 = illegal, 3 = user
uint16_t SR0, SR1, SR2, SR3;

void errorSR0(const uint16_t a, const uint8_t user)
{
    SR0 |= (a >> 12) & ~1;  // page no.

    SR0 |= (user << 6);  //  mode

    SR2 = procNS::curPC;
}

uint32_t decode_instr(const uint16_t a, const bool w, const uint8_t user)
{
    // disabled
    if (!(SR0 & 1))
    {
        if (a >= 0170000)
            return (uint32_t)((uint32_t)a + 0600000);
        return a;
    }

    // enabled

    const uint16_t i = (a >> 13);
    const uint16_t block = (a >> 6) & 0177;
    const uint16_t disp = a & 077;

    if (w && !instr_pages[user][i].write())  // write to RO page
    {
        SR0 = (1 << 13) | 1;  // abort RO
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            Serial.print(F("%% kt11::decode write to read-only page 0"));
            Serial.println(a, OCT);
            _printf("%%%% page %i, user %c, instr area\r\n", i, users_char[user]);
        }
        longjmp(trapbuf, INTMMUERR);
    }
    if (!instr_pages[user][i].read())  // read from WO page
    {
        SR0 = (1 << 15) | 1;  //abort non-resident
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            Serial.print(F("%% kt11::decode read from no-access page 0"));
            Serial.println(a, OCT);
            _printf("%%%% page %i, user %c, instr area\r\n", i, users_char[user]);
        }
        longjmp(trapbuf, INTMMUERR);
    }
    if (instr_pages[user][i].ed() && (block < instr_pages[user][i].len()))
    {
        SR0 = (1 << 14) | 1;  //abort page len error
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            _printf("%%%% page %i length exceeded (down).\r\n", i);
            _printf("%%%% address 0%06o; block 0%03o is below length 0%03o\r\n", a, block, (instr_pages[user][i].len()));
            _printf("%%%% user %c, instr area\r\n", users_char[user]);
        }
        longjmp(trapbuf, INTMMUERR);
    }
    if (!instr_pages[user][i].ed() && block > instr_pages[user][i].len())
    {
        SR0 = (1 << 14) | 1;  //abort page len error
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            _printf("%%%% page %i length exceeded (up).\r\n", i);
            _printf("%%%% address 0%06o; block 0%03o is above length 0%03o\r\n", a, block, (instr_pages[user][i].len()));
            _printf("%%%% user %c, instr area\r\n", users_char[user]);
        }
        longjmp(trapbuf, INTMMUERR);
    }

    if (w)
        instr_pages[user][i].pdr |= 1 << 6;

    uint32_t aa = ((block + instr_pages[user][i].addr()) << 6) + disp;

#if DEBUG_MMU
    if (PRINTSIMLINES && DEBUG_MMU)
    {
        _printf("%%%% kt11: page %i, user %c, instr area\r\n", i, users_char[user]);
        Serial.print("%% decode: slow ");
        Serial.print(a, OCT);
        Serial.print(" -> ");
        Serial.println(aa, OCT);
    }
#endif

    return aa;
}

uint32_t decode_data(const uint16_t a, const bool w, const uint8_t user)
{
#if !STRICT_11_40
    // if disabled in sr3, use instr decode
    if (user == 3)
    {
        if (!(SR3 & 1))
        {
            return decode_instr(a, w, user);
        }
    }
    else if (user == 1)
    {
        if (!(SR3 & 2))
        {
            return decode_instr(a, w, user);
        }
    }
    else if (user == 0)
    {
        if (!(SR3 & 4))
        {
            return decode_instr(a, w, user);
        }
    }

    // enabled

    const uint16_t i = (a >> 13);
    const uint16_t block = (a >> 6) & 0177;
    const uint16_t disp = a & 077;

    if (w && !data_pages[user][i].write())  // write to RO page
    {
        SR0 = (1 << 13) | 1;  // abort RO
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            Serial.print(F("%% kt11::decode write to read-only page 0"));
            Serial.println(a, OCT);
            _printf("%%%% page %i, user %c, data area\r\n", i, users_char[user]);
        }
        longjmp(trapbuf, INTMMUERR);
    }
    if (!data_pages[user][i].read())  // read from WO page
    {
        SR0 = (1 << 15) | 1;  //abort non-resident
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            Serial.print(F("%% kt11::decode read from no-access page 0"));
            Serial.println(a, OCT);
            _printf("%%%% page %i, user %c, data area\r\n", i, users_char[user]);
        }
        longjmp(trapbuf, INTMMUERR);
    }
    if (data_pages[user][i].ed() && (block < data_pages[user][i].len()))
    {
        SR0 = (1 << 14) | 1;  //abort page len error
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            _printf("%%%% page %i length exceeded (down).\r\n", i);
            _printf("%%%% address 0%06o; block 0%03o is below length 0%03o\r\n", a, block, (instr_pages[user][i].len()));
            _printf("%%%% user %c, data area\r\n", users_char[user]);
        }
        longjmp(trapbuf, INTMMUERR);
    }
    if (!data_pages[user][i].ed() && block > data_pages[user][i].len())
    {
        SR0 = (1 << 14) | 1;  //abort page len error
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            _printf("%%%% page %i length exceeded (up).\r\n", i);
            _printf("%%%% address 0%06o; block 0%03o is above length 0%03o\r\n", a, block, (instr_pages[user][i].len()));
            _printf("%%%% user %c, data area\r\n", users_char[user]);
        }
        longjmp(trapbuf, INTMMUERR);
    }

    if (w)
        data_pages[user][i].pdr |= 1 << 6;

    uint32_t aa = ((block + data_pages[user][i].addr()) << 6) + disp;

#if DEBUG_MMU
    if (PRINTSIMLINES && DEBUG_MMU)
    {
        _printf("%%%% kt11: page %i, user %c, data area\r\n", i, users_char[user]);
        Serial.print("%% decode: slow ");
        Serial.print(a, OCT);
        Serial.print(" -> ");
        Serial.println(aa, OCT);
    }
#endif

    return aa;
#else
    return decode_instr(a, w, user);
#endif
}

uint16_t read16(const uint32_t a)
{
    uint8_t i = ((a & 017) >> 1);

    // ~~~ Instructions Space

    if ((a >= DEV_KER_INS_PDR_R0) && (a <= DEV_KER_INS_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr read: page %i, user %c, instr area\r\n", i, users_char[0]);
        return instr_pages[0][i].pdr;
    }
    if ((a >= DEV_KER_INS_PAR_R0) && (a <= DEV_KER_INS_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par read: page %i, user %c, instr area\r\n", i, users_char[0]);
        return instr_pages[0][i].par;
    }

#if !STRICT_11_40
    if ((a >= DEV_SUP_INS_PDR_R0) && (a <= DEV_SUP_INS_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr read: page %i, user %c, instr area\r\n", i, users_char[1]);
        return instr_pages[1][i].pdr;
    }
    if ((a >= DEV_SUP_INS_PAR_R0) && (a <= DEV_SUP_INS_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par read: page %i, user %c, instr area\r\n", i, users_char[1]);
        return instr_pages[1][i].par;
    }
#endif

    if ((a >= DEV_USR_INS_PDR_R0) && (a <= DEV_USR_INS_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr read: page %i, user %c, instr area\r\n", i, users_char[3]);
        return instr_pages[3][i].pdr;
    }
    if ((a >= DEV_USR_INS_PAR_R0) && (a <= DEV_USR_INS_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par read: page %i, user %c, instr area\r\n", i, users_char[3]);
        return instr_pages[3][i].par;
    }

    // ~~~ Data space

    if ((a >= DEV_KER_DAT_PDR_R0) && (a <= DEV_KER_DAT_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr read: page %i, user %c, data area\r\n", i, users_char[0]);
        return data_pages[0][i].pdr;
    }
    if ((a >= DEV_KER_DAT_PAR_R0) && (a <= DEV_KER_DAT_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par read: page %i, user %c, data area\r\n", i, users_char[0]);
        return data_pages[0][i].par;
    }

#if !STRICT_11_40
    if ((a >= DEV_SUP_DAT_PDR_R0) && (a <= DEV_SUP_DAT_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr read: page %i, user %c, data area\r\n", i, users_char[1]);
        return data_pages[1][i].pdr;
    }
    if ((a >= DEV_SUP_DAT_PAR_R0) && (a <= DEV_SUP_DAT_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par read: page %i, user %c, data area\r\n", i, users_char[1]);
        return data_pages[1][i].par;
    }
#endif

    if ((a >= DEV_USR_DAT_PDR_R0) && (a <= DEV_USR_DAT_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr read: page %i, user %c, data area\r\n", i, users_char[3]);
        return data_pages[3][i].pdr;
    }
    if ((a >= DEV_USR_DAT_PAR_R0) && (a <= DEV_USR_DAT_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par read: page %i, user %c, data area\r\n", i, users_char[3]);
        return data_pages[3][i].par;
    }

    if (PRINTSIMLINES)
    {
        Serial.print(F("%% kt11::read16 invalid read from "));
        Serial.println(a, OCT);
    }
    longjmp(trapbuf, INTBUS);
}

void write16(const uint32_t a, const uint16_t v)
{
    uint8_t i = ((a & 017) >> 1);

    // ~~~ Instructions space

    if ((a >= DEV_KER_INS_PDR_R0) && (a <= DEV_KER_INS_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr write: page %i, user %c, instr area\r\n", i, users_char[0]);
        instr_pages[0][i].pdr = v;
        instr_pages[0][i].pdr &= ~(1 << 6);
        return;
    }
    if ((a >= DEV_KER_INS_PAR_R0) && (a <= DEV_KER_INS_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par write: page %i, user %c, instr area\r\n", i, users_char[0]);
        instr_pages[0][i].par = v;
        instr_pages[0][i].pdr &= ~(1 << 6);
        return;
    }

#if !STRICT_11_40
    if ((a >= DEV_SUP_INS_PDR_R0) && (a <= DEV_SUP_INS_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr write: page %i, user %c, instr area\r\n", i, users_char[1]);
        instr_pages[1][i].pdr = v;
        instr_pages[1][i].pdr &= ~(1 << 6);
        return;
    }
    if ((a >= DEV_SUP_INS_PAR_R0) && (a <= DEV_SUP_INS_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par write: page %i, user %c, instr area\r\n", i, users_char[1]);
        instr_pages[1][i].par = v;
        instr_pages[1][i].pdr &= ~(1 << 6);
        return;
    }
#endif

    if ((a >= DEV_USR_INS_PDR_R0) && (a <= DEV_USR_INS_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr write: page %i, user %c, instr area\r\n", i, users_char[3]);
        instr_pages[3][i].pdr = v;
        instr_pages[3][i].pdr &= ~(1 << 6);
        return;
    }
    if ((a >= DEV_USR_INS_PAR_R0) && (a <= DEV_USR_INS_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par write: page %i, user %c, instr area\r\n", i, users_char[3]);
        instr_pages[3][i].par = v;
        instr_pages[3][i].pdr &= ~(1 << 6);
        return;
    }

    // ~~~ Data space

    if ((a >= DEV_KER_DAT_PDR_R0) && (a <= DEV_KER_DAT_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr write: page %i, user %c, data area\r\n", i, users_char[0]);
        data_pages[0][i].pdr = v;
        data_pages[0][i].pdr &= ~(1 << 6);
        return;
    }
    if ((a >= DEV_KER_DAT_PAR_R0) && (a <= DEV_KER_DAT_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par write: page %i, user %c, data area\r\n", i, users_char[0]);
        data_pages[0][i].par = v;
        data_pages[0][i].pdr &= ~(1 << 6);
        return;
    }

#if !STRICT_11_40
    if ((a >= DEV_SUP_DAT_PDR_R0) && (a <= DEV_SUP_DAT_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr write: page %i, user %c, data area\r\n", i, users_char[1]);
        data_pages[1][i].pdr = v;
        data_pages[1][i].pdr &= ~(1 << 6);
        return;
    }
    if ((a >= DEV_SUP_DAT_PAR_R0) && (a <= DEV_SUP_DAT_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par write: page %i, user %c, data area\r\n", i, users_char[1]);
        data_pages[1][i].par = v;
        data_pages[1][i].pdr &= ~(1 << 6);
        return;
    }
#endif

    if ((a >= DEV_USR_DAT_PDR_R0) && (a <= DEV_USR_DAT_PDR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: pdr write: page %i, user %c, data area\r\n", i, users_char[3]);
        data_pages[3][i].pdr = v;
        data_pages[3][i].pdr &= ~(1 << 6);
        return;
    }
    if ((a >= DEV_USR_DAT_PAR_R0) && (a <= DEV_USR_DAT_PAR_R7))
    {
        if (PRINTSIMLINES && DEBUG_MMU)
            _printf("%%%% kt11: par write: page %i, user %c, data area\r\n", i, users_char[3]);
        data_pages[3][i].par = v;
        data_pages[3][i].pdr &= ~(1 << 6);
        return;
    }

    if (PRINTSIMLINES)
    {
        Serial.print(F("%% kt11::write16 0"));
        Serial.print(v, OCT);
        Serial.print(F(" from invalid address 0"));
        Serial.println(a, OCT);
    }
    longjmp(trapbuf, INTBUS);
}

};  // namespace kt11
