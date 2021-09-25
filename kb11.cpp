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

// sam11 software emulation of DEC kb11-A processor
// Mostly 11/40 kb11-A with KE11/KG11 extensions from 11/45 KB11-B

#include "kb11.h"

#if USE_11_45

#include "bootrom.h"
#include "dd11.h"
#include "kl11.h"
#include "kt11.h"
#include "kw11.h"
#include "ky11.h"
#include "ms11.h"
#include "platform.h"
#include "rk11.h"
#include "sam11.h"

#include <SdFat.h>

pdp11::intr itab[ITABN];

namespace kb11 {

// signed integer registers
volatile int32_t R[8];  // R6 = SP, R7 = PC

volatile uint16_t PS;     // Processor Status
volatile uint16_t curPC;  // R7, address of current instruction
volatile uint16_t KSP;    // R6 (kernel), stack pointer
volatile uint16_t USP;    // R6 (user), stack pointer
volatile uint16_t SSP;    // R6 (Super), stack pointer

volatile uint8_t curuser;   // 0: kernel, 1: supervisor, 2: illegal, 3: user
volatile uint8_t prevuser;  // 0: kernel, 1: supervisor, 2: illegal, 3: user

volatile bool trapped = false;
volatile bool cont_with = false;
volatile bool waiting = false;

void reset(void)
{
    ky11::reset();
    uint16_t i;
    for (i = 0; i < 7; i++)
    {
        R[i] = 0;
    }
    kt11::SLR = 0400;
    PS = 0;
    KSP = 0;
    SSP = 0;
    USP = 0;
    curuser = false;
    prevuser = false;
    kt11::SR0 = 0;
    curPC = 0;
    kw11::reset();
    ms11::clear();
    for (i = 0; i < BOOT_LEN; i++)
    {
        dd11::write16(BOOT_START + (i * 2), bootrom_rk0[i]);
    }
    R[7] = BOOT_START;
    kl11::clearterminal();
    rk11::reset();
    waiting = false;

#ifdef PIN_OUT_PROC_RUN
    digitalWrite(PIN_OUT_PROC_RUN, LED_ON);
#endif
}

static uint16_t read8(const uint16_t a)
{
    return dd11::read8(kt11::decode_instr(a, false, curuser));
}

static uint16_t read16(const uint16_t a)
{
    return dd11::read16(kt11::decode_instr(a, false, curuser));
}

static void write8(const uint16_t a, const uint16_t v)
{
    dd11::write8(kt11::decode_instr(a, true, curuser), v);
}

static void write16(const uint16_t a, const uint16_t v)
{
    dd11::write16(kt11::decode_instr(a, true, curuser), v);
}

bool isReg(const uint16_t a)
{
    return (a & 0177770) == 0170000;
}

static uint16_t memread16(const uint16_t a)
{
    if (isReg(a))
    {
        return R[a & 7];
    }
    return read16(a);
}

static uint16_t memread(uint16_t a, uint8_t l)
{
    if (isReg(a))
    {
        const uint8_t r = a & 7;
        if (l == 2)
        {
            return R[r];
        }
        else
        {
            return R[r] & 0xFF;
        }
    }
    if (l == 2)
    {
        return read16(a);
    }
    return read8(a);
}

static void memwrite16(const uint16_t a, const uint16_t v)
{
    if (isReg(a))
    {
        R[a & 7] = v;
    }
    else
    {
        write16(a, v);
    }
}

static void memwrite(const uint16_t a, const uint8_t l, const uint16_t v)
{
    if (isReg(a))
    {
        const uint8_t r = a & 7;
        if (l == 2)
        {
            R[r] = v;
        }
        else
        {
            R[r] &= 0xFF00;
            R[r] |= v;
        }
        return;
    }
    if (l == 2)
    {
        write16(a, v);
    }
    else
    {
        write8(a, v);
    }
}

static uint16_t fetch16()
{
    const uint16_t val = read16(R[7]);
    R[7] += 2;
    return val;
}

static void push(const uint16_t v)
{
    R[6] -= 2;
    write16(R[6], v);
}

static uint16_t pop()
{
    const uint16_t val = read16(R[6]);
    R[6] += 2;
    return val;
}

// aget resolves the operand to a vaddress.
// if the operand is a register, an address in
// the range [0170000,0170007). This address range is
// technically a valid IO page, but dd11 doesn't map
// any addresses here, so we can safely do this.
static uint16_t aget(uint8_t v, uint8_t l)
{
    uint16_t addr = 0;
    if (((v & 7) >= 6) || (v & 010))
    {
        l = 2;
    }
    if ((v & 070) == 000)
    {
        return 0170000 | (v & 7);
    }
    switch (v & 060)
    {
    case 000:
        v &= 07;
        addr = R[v & 07];
        break;
    case 020:
        addr = R[v & 07];
        R[v & 07] += l;
        break;
    case 040:
        R[v & 07] -= l;
        addr = R[v & 07];
        break;
    case 060:
        addr = fetch16();
        addr += R[v & 07];
        break;
    }
    //addr &= 0xFFFF;  // ?
    if (v & 010)
    {
        addr = read16(addr);
    }
    return addr;
}

static void branch(int16_t o)
{
    if (o & 0x80)
    {
        o = -(((~o) + 1) & 0xFF);
    }
    o <<= 1;
    R[7] += o;
}

void switchmode(uint8_t newm)
{
    prevuser = curuser;
    curuser = newm;
    if (prevuser == 3)
    {
        USP = R[6];
    }
    else if (prevuser == 1)
    {
        SSP = R[6];
    }
    else
    {
        KSP = R[6];
    }
    if (curuser == 3)
    {
#ifdef PIN_OUT_USER_MODE
        digitalWrite(PIN_OUT_USER_MODE, LED_ON);
#endif
#ifdef PIN_OUT_SUPER_MODE
        digitalWrite(PIN_OUT_SUPER_MODE, LED_OFF);
#endif
#ifdef PIN_OUT_KERNEL_MODE
        digitalWrite(PIN_OUT_KERNEL_MODE, LED_OFF);
#endif
        R[6] = USP;
    }
    if (curuser == 1)
    {
#ifdef PIN_OUT_SUPER_MODE
        digitalWrite(PIN_OUT_SUPER_MODE, LED_ON);
#endif
#ifdef PIN_OUT_USER_MODE
        digitalWrite(PIN_OUT_USER_MODE, LED_OFF);
#endif
#ifdef PIN_OUT_KERNEL_MODE
        digitalWrite(PIN_OUT_KERNEL_MODE, LED_OFF);
#endif
        R[6] = SSP;
    }
    else
    {
#ifdef PIN_OUT_USER_MODE
        digitalWrite(PIN_OUT_USER_MODE, LED_OFF);
#endif
#ifdef PIN_OUT_SUPER_MODE
        digitalWrite(PIN_OUT_SUPER_MODE, LED_OFF);
#endif
#ifdef PIN_OUT_KERNEL_MODE
        digitalWrite(PIN_OUT_KERNEL_MODE, LED_ON);
#endif
        R[6] = KSP;
    }
    PS &= 0007777;
    PS |= (curuser << 14);
    PS |= (prevuser << 12);
}

static void setZ(bool b)
{
    if (b)
        PS |= FLAGZ;
}

static void MOV(const uint16_t instr)
{
    const uint8_t d = instr & 077;
    const uint8_t s = (instr & 07700) >> 6;
    uint8_t l = 2 - (instr >> 15);
    const uint16_t msb = l == 2 ? 0x8000 : 0x80;
    const uint16_t sa = aget(s, l);
    uint16_t uval = memread(sa, l);
    const uint16_t da = aget(d, l);

    PS &= 0xFFF1;
    if (uval & msb)
    {
        PS |= FLAGN;
    }
    if (uval == 0)
    {
        PS |= FLAGZ;
    }

    if ((isReg(da)) && (l == 1))
    {
        l = 2;
        if (uval & msb)
        {
            uval |= 0xFF00;
        }
    }
    memwrite(da, l, uval);
}

static void CMP(uint16_t instr)
{
    const uint8_t d = instr & 077;
    const uint8_t s = (instr & 07700) >> 6;
    const uint8_t l = 2 - (instr >> 15);
    const uint16_t msb = l == 2 ? 0x8000 : 0x80;
    const uint16_t max = l == 2 ? 0xFFFF : 0xff;
    uint16_t val1 = memread(aget(s, l), l);
    const uint16_t da = aget(d, l);
    uint16_t val2 = memread(da, l);
    const int32_t sval = (val1 - val2) & max;
    PS &= 0xFFF0;
    setZ(sval == 0);
    if (sval & msb)
    {
        PS |= FLAGN;
    }
    if (((val1 ^ val2) & msb) && (!((val2 ^ sval) & msb)))
    {
        PS |= FLAGV;
    }
    if (val1 < val2)
    {
        PS |= FLAGC;
    }
}

static void BIT(uint16_t instr)
{
    const uint8_t d = instr & 077;
    const uint8_t s = (instr & 07700) >> 6;
    const uint8_t l = 2 - (instr >> 15);
    const uint16_t msb = l == 2 ? 0x8000 : 0x80;
    const uint16_t val1 = memread(aget(s, l), l);
    const uint16_t da = aget(d, l);
    const uint16_t val2 = memread(da, l);
    const uint16_t uval = val1 & val2;
    PS &= 0xFFF1;
    setZ(uval == 0);
    if (uval & msb)
    {
        PS |= FLAGN;
    }
}

static void BIC(uint16_t instr)
{
    const uint8_t d = instr & 077;
    const uint8_t s = (instr & 07700) >> 6;
    const uint8_t l = 2 - (instr >> 15);
    const uint16_t msb = l == 2 ? 0x8000 : 0x80;
    const uint16_t max = l == 2 ? 0xFFFF : 0xff;
    const uint16_t val1 = memread(aget(s, l), l);
    const uint16_t da = aget(d, l);
    const uint16_t val2 = memread(da, l);
    const uint16_t uval = (max ^ val1) & val2;
    PS &= 0xFFF1;
    setZ(uval == 0);
    if (uval & msb)
    {
        PS |= FLAGN;
    }
    memwrite(da, l, uval);
}

static void BIS(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t s = (instr & 07700) >> 6;
    uint8_t l = 2 - (instr >> 15);
    uint16_t msb = l == 2 ? 0x8000 : 0x80;
    uint16_t val1 = memread(aget(s, l), l);
    uint16_t da = aget(d, l);
    uint16_t val2 = memread(da, l);
    uint16_t uval = val1 | val2;
    PS &= 0xFFF1;
    setZ(uval == 0);
    if (uval & msb)
    {
        PS |= FLAGN;
    }
    memwrite(da, l, uval);
}

static void ADD(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t s = (instr & 07700) >> 6;
    //uint8_t l = 2 - (instr >> 15);
    uint16_t sa = aget(s, 2);
    uint16_t val1 = memread16(sa);
    uint16_t da = aget(d, 2);
    uint16_t val2 = memread16(da);
    uint16_t uval = (val1 + val2) & 0xFFFF;
    PS &= 0xFFF0;
    setZ(uval == 0);
    if (uval & 0x8000)
    {
        PS |= FLAGN;
    }
    if (!((val1 ^ val2) & 0x8000) && ((val2 ^ uval) & 0x8000))
    {
        PS |= FLAGV;
    }
    if ((val1 + val2) >= 0xFFFF)
    {
        PS |= FLAGC;
    }
    memwrite16(da, uval);
}

static void SUB(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t s = (instr & 07700) >> 6;
    //uint8_t l = 2 - (instr >> 15);
    uint16_t sa = aget(s, 2);
    uint16_t val1 = memread16(sa);
    uint16_t da = aget(d, 2);
    uint16_t val2 = memread16(da);
    uint16_t uval = (val2 - val1) & 0xFFFF;
    PS &= 0xFFF0;
    setZ(uval == 0);
    if (uval & 0x8000)
    {
        PS |= FLAGN;
    }
    if (((val1 ^ val2) & 0x8000) && (!((val2 ^ uval) & 0x8000)))
    {
        PS |= FLAGV;
    }
    if (val1 > val2)
    {
        PS |= FLAGC;
    }
    memwrite16(da, uval);
}

static void JSR(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t s = (instr & 07700) >> 6;
    uint8_t l = 2 - (instr >> 15);
    uint16_t uval = aget(d, l);
    if (isReg(uval))
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% JSR called on register"));
        }
        panic();
    }
    push(R[s & 7]);
    R[s & 7] = R[7];
    R[7] = uval;
}

static void MUL(const uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t s = (instr & 07700) >> 6;
    int32_t val1 = R[s & 7];
    if (val1 & 0x8000)
    {
        val1 = -((0xFFFF ^ val1) + 1);
    }
    uint8_t l = 2 - (instr >> 15);
    uint16_t da = aget(d, l);
    int32_t val2 = memread16(da);
    if (val2 & 0x8000)
    {
        val2 = -((0xFFFF ^ val2) + 1);
    }
    int32_t sval = val1 * val2;
    R[s & 7] = sval >> 16;
    R[(s & 7) | 1] = sval & 0xFFFF;
    PS &= 0xFFF0;
    if (sval & 0x80000000)
    {
        PS |= FLAGN;
    }
    setZ((sval & 0xFFFFFFFF) == 0);
    if ((sval < 0x8000) || (sval >= 0x7FFF))  // if ((sval < (1 << 15)) || (sval >= ((1 << 15) - 1)))
    {
        PS |= FLAGC;
    }
}

static void DIV(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t s = (instr & 07700) >> 6;
    int32_t val1 = (R[s & 7] << 16) | (R[(s & 7) | 1]);
    uint8_t l = 2 - (instr >> 15);
    uint16_t da = aget(d, l);
    int32_t val2 = memread16(da);
    PS &= 0xFFF0;
    if (val2 == 0)
    {
        PS |= FLAGC;
        return;
    }
    if ((val1 / val2) >= 0x10000)
    {
        PS |= FLAGV;
        return;
    }
    R[s & 7] = (val1 / val2) & 0xFFFF;
    R[(s & 7) | 1] = (val1 % val2) & 0xFFFF;
    setZ(R[s & 7] == 0);
    if (R[s & 7] & 0100000)
    {
        PS |= FLAGN;
    }
    if (val1 == 0)
    {
        PS |= FLAGV;
    }
}

static void ASH(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t s = (instr & 07700) >> 6;
    uint16_t val1 = R[s & 7];
    uint16_t da = aget(d, 2);
    uint16_t val2 = memread16(da) & 077;
    PS &= 0xFFF0;
    int32_t sval;
    if (val2 & 040)
    {
        val2 = (077 ^ val2) + 1;
        if (val1 & 0100000)
        {
            sval = 0xFFFF ^ (0xFFFF >> val2);
            sval |= val1 >> val2;
        }
        else
        {
            sval = val1 >> val2;
        }
        if (val1 & (1 << (val2 - 1)))
        {
            PS |= FLAGC;
        }
    }
    else
    {
        sval = (val1 << val2) & 0xFFFF;
        if (val1 & (1 << (16 - val2)))
        {
            PS |= FLAGC;
        }
    }
    R[s & 7] = sval;
    setZ(sval == 0);
    if (sval & 0100000)
    {
        PS |= FLAGN;
    }
    if ((sval & 0100000) xor (val1 & 0100000))
    {
        PS |= FLAGV;
    }
}

static void ASHC(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t s = (instr & 07700) >> 6;
    uint16_t val1 = R[s & 7] << 16 | R[(s & 7) | 1];
    uint16_t da = aget(d, 2);
    uint16_t val2 = memread16(da) & 077;
    PS &= 0xFFF0;
    int32_t sval;
    if (val2 & 040)
    {
        val2 = (077 ^ val2) + 1;
        if (val1 & 0x80000000)
        {
            sval = 0xFFFFFFFF ^ (0xFFFFFFFF >> val2);
            sval |= val1 >> val2;
        }
        else
        {
            sval = val1 >> val2;
        }
        if (val1 & (1 << (val2 - 1)))
        {
            PS |= FLAGC;
        }
    }
    else
    {
        sval = (val1 << val2) & 0xFFFFFFFF;
        if (val1 & (1 << (32 - val2)))
        {
            PS |= FLAGC;
        }
    }
    R[s & 7] = (sval >> 16) & 0xFFFF;
    R[(s & 7) | 1] = sval & 0xFFFF;
    setZ(sval == 0);
    if (sval & 0x80000000)
    {
        PS |= FLAGN;
    }
    if ((sval & 0x80000000) xor (val1 & 0x80000000))
    {
        PS |= FLAGV;
    }
}

static void XOR(uint16_t instr)
{
    const uint8_t d = instr & 077;
    const uint8_t s = (instr & 07700) >> 6;
    const uint16_t val1 = R[s & 7];
    const uint16_t da = aget(d, 2);
    const uint16_t val2 = memread16(da);
    const uint16_t uval = val1 ^ val2;
    PS &= 0xFFF1;
    setZ(uval == 0);
    if (uval & 0x8000)
    {
        PS |= FLAGN;
    }
    memwrite16(da, uval);
}

static void SOB(const uint16_t instr)
{
    // 0077Roo

    uint8_t s = (instr & 0000700) >> 6;
    uint8_t o = instr & 0000077;  //0xFF;
    R[s]--;                       // & 07]--;
    if (R[s])                     // & 07])  //71
    {
        //o &= 077;
        //o <<= 1;
        o *= 2;
        R[7] -= o;
    }
}

static void CLR(uint16_t instr)
{
    // 0c050DD where c is 0/1 depending on if register

    const uint8_t d = instr & 077;
    const uint8_t l = 2 - (instr >> 15);
    //PS &= 0xFFF0;
    PS &= ~FLAGN;
    PS &= ~FLAGV;
    PS &= ~FLAGC;
    PS |= FLAGZ;

    uint16_t da = aget(d, l);
    memwrite(da, l, 0);
}

static void COM(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t s = (instr & 07700) >> 6;
    uint8_t l = 2 - (instr >> 15);
    uint16_t msb = l == 2 ? 0x8000 : 0x80;
    uint16_t max = l == 2 ? 0xFFFF : 0xff;
    uint16_t da = aget(d, l);
    uint16_t uval = memread(da, l) ^ max;
    PS &= 0xFFF0;
    PS |= FLAGC;
    if (uval & msb)
    {
        PS |= FLAGN;
    }
    setZ(uval == 0);
    memwrite(da, l, uval);
}

static void INC(const uint16_t instr)
{
    const uint8_t d = instr & 077;
    const uint8_t l = 2 - (instr >> 15);
    const uint16_t msb = l == 2 ? 0x8000 : 0x80;
    const uint16_t max = l == 2 ? 0xFFFF : 0xff;
    const uint16_t da = aget(d, l);
    const uint16_t uval = (memread(da, l) + 1) & max;
    PS &= 0xFFF1;
    if (uval & msb)
    {
        PS |= FLAGN | FLAGV;
    }
    setZ(uval == 0);
    memwrite(da, l, uval);
}

static void _DEC(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    uint16_t msb = l == 2 ? 0x8000 : 0x80;
    uint16_t max = l == 2 ? 0xFFFF : 0xff;
    uint16_t maxp = l == 2 ? 0x7FFF : 0x7f;
    uint16_t da = aget(d, l);
    uint16_t uval = (memread(da, l) - 1) & max;
    PS &= 0xFFF1;
    if (uval & msb)
    {
        PS |= FLAGN;
    }
    if (uval == maxp)
    {
        PS |= FLAGV;
    }
    setZ(uval == 0);
    memwrite(da, l, uval);
}

static void NEG(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    uint16_t msb = l == 2 ? 0x8000 : 0x80;
    uint16_t max = l == 2 ? 0xFFFF : 0xff;
    uint16_t da = aget(d, l);
    int32_t sval = (-memread(da, l)) & max;
    PS &= 0xFFF0;
    if (sval & msb)
    {
        PS |= FLAGN;
    }
    if (sval == 0)
    {
        PS |= FLAGZ;
    }
    else
    {
        PS |= FLAGC;
    }
    if (sval == 0x8000)
    {
        PS |= FLAGV;
    }
    memwrite(da, l, sval);
}

static void _ADC(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    uint16_t msb = l == 2 ? 0x8000 : 0x80;
    uint16_t max = l == 2 ? 0xFFFF : 0xff;
    uint16_t da = aget(d, l);
    uint16_t uval = memread(da, l);
    if (PS & FLAGC)
    {
        PS &= 0xFFF0;
        if ((uval + 1) & msb)
        {
            PS |= FLAGN;
        }
        setZ(uval == max);
        if (uval == 0077777)
        {
            PS |= FLAGV;
        }
        if (uval == 0177777)
        {
            PS |= FLAGC;
        }
        memwrite(da, l, (uval + 1) & max);
    }
    else
    {
        PS &= 0xFFF0;
        if (uval & msb)
        {
            PS |= FLAGN;
        }
        setZ(uval == 0);
    }
}

static void SBC(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    uint16_t msb = l == 2 ? 0x8000 : 0x80;
    uint16_t max = l == 2 ? 0xFFFF : 0xff;
    uint16_t da = aget(d, l);
    int32_t sval = memread(da, l);
    if (PS & FLAGC)
    {
        PS &= 0xFFF0;
        if ((sval - 1) & msb)
        {
            PS |= FLAGN;
        }
        setZ(sval == 1);
        if (sval)
        {
            PS |= FLAGC;
        }
        if (sval == 0100000)
        {
            PS |= FLAGV;
        }
        memwrite(da, l, (sval - 1) & max);
    }
    else
    {
        PS &= 0xFFF0;
        if (sval & msb)
        {
            PS |= FLAGN;
        }
        setZ(sval == 0);
        if (sval == 0100000)
        {
            PS |= FLAGV;
        }
        PS |= FLAGC;
    }
}

static void TST(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    uint16_t msb = l == 2 ? 0x8000 : 0x80;
    uint16_t uval = memread(aget(d, l), l);
    PS &= 0xFFF0;
    if (uval & msb)
    {
        PS |= FLAGN;
    }
    setZ(uval == 0);
}

static void ROR(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    int32_t max = l == 2 ? 0xFFFF : 0xff;
    uint16_t da = aget(d, l);
    int32_t sval = memread(da, l);
    if (PS & FLAGC)
    {
        sval |= max + 1;
    }
    PS &= 0xFFF0;
    if (sval & 1)
    {
        PS |= FLAGC;
    }
    // watch out for integer wrap around
    if (sval & (max + 1))
    {
        PS |= FLAGN;
    }
    setZ(!(sval & max));
    if ((sval & 1) xor (sval & (max + 1)))
    {
        PS |= FLAGV;
    }
    sval >>= 1;
    memwrite(da, l, sval);
}

static void ROL(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    uint16_t msb = l == 2 ? 0x8000 : 0x80;
    int32_t max = l == 2 ? 0xFFFF : 0xff;
    uint16_t da = aget(d, l);
    int32_t sval = memread(da, l) << 1;
    if (PS & FLAGC)
    {
        sval |= 1;
    }
    PS &= 0xFFF0;
    if (sval & (max + 1))
    {
        PS |= FLAGC;
    }
    if (sval & msb)
    {
        PS |= FLAGN;
    }
    setZ(!(sval & max));
    if ((sval ^ (sval >> 1)) & msb)
    {
        PS |= FLAGV;
    }
    sval &= max;
    memwrite(da, l, sval);
}

static void ASR(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    uint16_t msb = l == 2 ? 0x8000 : 0x80;
    uint16_t da = aget(d, l);
    uint16_t uval = memread(da, l);
    PS &= 0xFFF0;
    if (uval & 1)
    {
        PS |= FLAGC;
    }
    if (uval & msb)
    {
        PS |= FLAGN;
    }
    if ((uval & msb) xor (uval & 1))
    {
        PS |= FLAGV;
    }
    uval = (uval & msb) | (uval >> 1);
    setZ(uval == 0);
    memwrite(da, l, uval);
}

static void ASL(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    uint16_t msb = l == 2 ? 0x8000 : 0x80;
    uint16_t max = l == 2 ? 0xFFFF : 0xff;
    uint16_t da = aget(d, l);
    // TODO(dfc) doesn't need to be an sval
    int32_t sval = memread(da, l);
    PS &= 0xFFF0;
    if (sval & msb)
    {
        PS |= FLAGC;
    }
    if (sval & (msb >> 1))
    {
        PS |= FLAGN;
    }
    if ((sval ^ (sval << 1)) & msb)
    {
        PS |= FLAGV;
    }
    sval = (sval << 1) & max;
    setZ(sval == 0);
    memwrite(da, l, sval);
}

static void SXT(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    uint16_t max = l == 2 ? 0xFFFF : 0xff;
    uint16_t da = aget(d, l);
    if (PS & FLAGN)
    {
        memwrite(da, l, max);
    }
    else
    {
        PS |= FLAGZ;
        memwrite(da, l, 0);
    }
}

static void JMP(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint16_t uval = aget(d, 2);
    if (isReg(uval))
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% JMP called with register dest"));
        }
        panic();
    }
    R[7] = uval;
}

static void SWAB(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint8_t l = 2 - (instr >> 15);
    uint16_t da = aget(d, l);
    uint16_t uval = memread(da, l);
    uval = ((uval >> 8) | (uval << 8)) & 0xFFFF;
    PS &= 0xFFF0;
    setZ(uval & 0xFF);
    if (uval & 0x80)
    {
        PS |= FLAGN;
    }
    memwrite(da, l, uval);
}

static void MARK(uint16_t instr)
{
    R[6] = R[7] + ((instr & 077) << 1);
    R[7] = R[5];
    R[5] = pop();
}

static void MFPI(uint16_t instr)
{
    uint8_t d = instr & 077;
    uint16_t da = aget(d, 2);
    uint16_t uval;
    if (da == 0170006)
    {
        if (curuser == prevuser)
        {
            uval = R[6];
        }
        else
        {
            if (prevuser)
            {
                uval = USP;
            }
            else
            {
                uval = KSP;
            }
        }
    }
    else if (isReg(da))
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% invalid MFPI instruction"));
        }
        panic();
    }
    else
    {
        uval = dd11::read16(kt11::decode_instr((uint16_t)da, false, prevuser));
    }
    push(uval);
    PS &= 0xFFF0;
    PS |= FLAGC;
    setZ(uval == 0);
    if (uval & 0x8000)
    {
        PS |= FLAGN;
    }
}

static void MTPI(uint16_t instr)
{
    uint32_t sa = 0;
    uint8_t d = instr & 077;
    uint16_t da = aget(d, 2);
    uint16_t uval = pop();
    if (da == 0170006)
    {
        if (curuser == prevuser)
        {
            R[6] = uval;
        }
        else if (prevuser)
        {
            USP = uval;
        }
        else
        {
            KSP = uval;
        }
    }
    else if (isReg(da))
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% invalid MTPI instruction"));
        }
        panic();
    }
    else
    {
        sa = kt11::decode_instr(da, true, prevuser);
        dd11::write16(sa, uval);
    }
    PS &= 0xFFF0;
    PS |= FLAGC;
    setZ(uval == 0);
    if (uval & 0x8000)
    {
        PS |= FLAGN;
    }
    return;
}

static void RTS(uint16_t instr)
{
    uint8_t d = instr & 077;
    R[7] = R[d & 7];
    R[d & 7] = pop();
}

static void EMTX(uint16_t instr)
{
    uint16_t uval;
    if ((instr & 0177400) == 0104000)
    {
        uval = 030;
    }
    else if ((instr & 0177400) == 0104400)
    {
        uval = 034;
    }
    else if (instr == 3)
    {
        uval = 014;
    }
    else
    {
        uval = 020;
    }
    uint16_t prev = PS;
    switchmode(0);
    push(prev);
    push(R[7]);
    R[7] = dd11::read16(uval);
    PS = dd11::read16(uval + 2);
    if (prevuser)
    {
        PS |= (1 << 13) | (1 << 12);
    }
}

static void RTT(uint16_t instr)
{
    R[7] = pop();
    uint16_t uval = pop();
    if (curuser)
    {
        uval &= 047;
        uval |= PS & 0177730;
    }
    dd11::write16(0777776, uval);
}

static void RESET(uint16_t instr)
{
    if (curuser)
    {
        return;
    }
    kl11::clearterminal();
    rk11::reset();
}

void step()
{
    if (waiting)
        return;

    if (PRINTSIMLINES)
    {
        if (BREAK_ON_TRAP && trapped)
        {
            //Serial.print("!");
            //printstate();

            Serial.print("\r\n%%!");
            while (!Serial.available())
                delay(1);
            char c;
            while (1)
            {
                c = Serial.read();
                if (c == '`')  // step individually
                {
                    trapped = true;
                    cont_with = false;
                    break;
                }
                if (c == '>')  // continue to the next trap
                {
                    trapped = false;
                    cont_with = false;
                    break;
                }
                if (c == '~')  // continue to the next trap, but keep printing
                {
                    trapped = false;
                    cont_with = true;
                    break;
                }
                if (c == 'd' && ALLOW_DISASM)
                {
                    trapped = false;
                    cont_with = false;
                    disasm(kt11::decode_instr(kb11::curPC, false, kb11::curuser));
                    break;
                }
                if (c == 'a')
                {
                    printstate();
                }
            }
            Serial.print(c);
            Serial.println();
        }

        if ((BREAK_ON_TRAP || PRINTINSTR || PRINTSTATE) && (cont_with || trapped))
        {
            delayMicroseconds(100);
        }
    }

    curPC = R[7];
    uint16_t instr = dd11::read16(kt11::decode_instr(R[7], false, curuser));
    // return;
    R[7] += 2;

    if (PRINTSIMLINES)
    {
        if (PRINTINSTR && (trapped || cont_with))
        {
            _printf("%%%% instr 0%06o: 0%06o\r\n", kb11::curPC, dd11::read16(kt11::decode_instr(kb11::curPC, false, kb11::curuser)));
        }

        if ((BREAK_ON_TRAP || PRINTSTATE) && (trapped || cont_with))
            printstate();
    }

    switch ((instr >> 12) & 007)
    {
    case 001:  // MOV
        MOV(instr);
        return;
    case 002:  // CMP
        CMP(instr);
        return;
    case 003:  // BIT
        BIT(instr);
        return;
    case 004:  // BIC
        BIC(instr);
        return;
    case 005:  // BIS
        BIS(instr);
        return;
    }
    switch ((instr >> 12) & 017)
    {
    case 006:  // ADD
        ADD(instr);
        return;
    case 016:  // SUB
        SUB(instr);
        return;
    }
    switch ((instr >> 9) & 0177)
    {
    case 0004:  // JSR
        JSR(instr);
        return;
    case 0070:  // MUL
        MUL(instr);
        return;
    case 0071:  // DIV
        DIV(instr);
        return;
    case 0072:  // ASH
        ASH(instr);
        return;
    case 0073:  // ASHC
        ASHC(instr);
        return;
    case 0074:  // XOR
        XOR(instr);
        return;
    case 0077:  // SOB
        SOB(instr);
        return;
    }
    switch ((instr >> 6) & 00777)
    {
    case 00050:  // CLR
        CLR(instr);
        return;
    case 00051:  // COM
        COM(instr);
        return;
    case 00052:  // INC
        INC(instr);
        return;
    case 00053:  // DEC
        _DEC(instr);
        return;
    case 00054:  // NEG
        NEG(instr);
        return;
    case 00055:  // ADC
        _ADC(instr);
        return;
    case 00056:  // SBC
        SBC(instr);
        return;
    case 00057:  // TST
        TST(instr);
        return;
    case 00060:  // ROR
        ROR(instr);
        return;
    case 00061:  // ROL
        ROL(instr);
        return;
    case 00062:  // ASR
        ASR(instr);
        return;
    case 00063:  // ASL
        ASL(instr);
        return;
    case 00067:  // SXT
        SXT(instr);
        return;
    }
    switch (instr & 0177700)
    {
    case 0000100:  // JMP
        JMP(instr);
        return;
    case 0000300:  // SWAB
        SWAB(instr);
        return;
    case 0006400:  // MARK
        MARK(instr);
        break;
    case 0006500:  // MFPI
        MFPI(instr);
        return;
    case 0006600:  // MTPI
        MTPI(instr);
        return;
        // case 0106500:  // MFPD
        //     MFPD(instr);
        //     return;
        // case 0106600:  // MTPD
        //     MTPD(instr);
        //     return;
        // case 0106700:  // MFPS
        //     MFPS(instr);
        //     return;
        // case 0106400:  // MTPS
        //     MTPS(instr);
        //     return;
    }
    if ((instr & 0177770) == 0000200)
    {  // RTS
        RTS(instr);
        return;
    }

    switch (instr & 0177400)
    {
    case 0000400:
        branch(instr & 0xFF);
        return;
    case 0001000:
        if (!Z())
        {
            branch(instr & 0xFF);
        }
        return;
    case 0001400:
        if (Z())
        {
            branch(instr & 0xFF);
        }
        return;
    case 0002000:
        if (!(N() xor V()))
        {
            branch(instr & 0xFF);
        }
        return;
    case 0002400:
        if (N() xor V())
        {
            branch(instr & 0xFF);
        }
        return;
    case 0003000:
        if ((!(N() xor V())) && (!Z()))
        {
            branch(instr & 0xFF);
        }
        return;
    case 0003400:
        if ((N() xor V()) || Z())
        {
            branch(instr & 0xFF);
        }
        return;
    case 0100000:
        if (!N())
        {
            branch(instr & 0xFF);
        }
        return;
    case 0100400:
        if (N())
        {
            branch(instr & 0xFF);
        }
        return;
    case 0101000:
        if ((!C()) && (!Z()))
        {
            branch(instr & 0xFF);
        }
        return;
    case 0101400:
        if (C() || Z())
        {
            branch(instr & 0xFF);
        }
        return;
    case 0102000:
        if (!V())
        {
            branch(instr & 0xFF);
        }
        return;
    case 0102400:
        if (V())
        {
            branch(instr & 0xFF);
        }
        return;
    case 0103000:
        if (!C())
        {
            branch(instr & 0xFF);
        }
        return;
    case 0103400:
        if (C())
        {
            branch(instr & 0xFF);
        }
        return;
    }
    // if (((instr & 0177000) == 0104000) || (instr == 3) || (instr == 4))
    // {  // EMT TRAP IOT BPT
    //     EMTX(instr);
    //     return;
    // }
    if ((instr & 0177740) == 0240)
    {  // CL?, SE?
        if (instr & 020)
        {
            PS |= instr & 017;
        }
        else
        {
            PS &= ~instr & 017;
        }
        return;
    }
    switch (instr & 7)
    {
    case 00:          // HALT
        if (curuser)  // modded, usually a HALT in user mode fails with a trap
        {
            break;
        }
        //Serial.println(F("%% HALT"));
        panic();
    case 01:  // WAIT
        if (curuser)
        {
            break;
        }
        waiting = true;
        return;
    case 03:  // BPT
    case 04:  // IOT
        EMTX(instr);
        return;
    case 02:  // RTI
    case 06:  // RTT
        RTT(instr);
        return;
    case 05:  // RESET
        RESET(instr);
        return;
        // case 07:
        //     MFPT(instr);  // 11/44 only
        //     return;
    }

    // FP11
    if ((instr & 0177000) == 0170000)
    {
        switch (instr)
        {
        case 0170001:  // SETF; Set floating mode
            return;
        case 0170002:  // SETI; Set integer mode
            return;
        case 0170011:  // SETD; Set double mode; not needed by UNIX, but used; therefore ignored
            return;
        case 0170012:  // SETL; Set long mode
            return;
        }
    }
    if (PRINTSIMLINES)
    {
        Serial.print("%% invalid instruction 0");
        Serial.println(instr, OCT);
    }
    longjmp(trapbuf, INTINVAL);
}

void trapat(uint16_t vec)
{
    if (vec & 1)
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% Thou darst calling trapat() with an odd vector number?"));
        }
        panic();
    }
    trapped = true;
    cont_with = false;
    if (PRINTSIMLINES)
    {
        Serial.print(F("%% trap: "));
        Serial.println(vec, OCT);

        if (DEBUG_TRAP)
        {
            _printf("%%%% R0 0%06o R1 0%06o R2 0%06o R3 0%06o\r\n",
              uint16_t(kb11::R[0]), uint16_t(kb11::R[1]), uint16_t(kb11::R[2]), uint16_t(kb11::R[3]));
            _printf("%%%% R4 0%06o R5 0%06o R6 0%06o R7 0%06o\r\n",
              uint16_t(kb11::R[4]), uint16_t(kb11::R[5]), uint16_t(kb11::R[6]), kb11::R[7]);  // uint16_t(kb11::R[7]));
        }
    }
    /*var prev uint16
       defer func() {
           t = recover()
           switch t = t.(type) {
           case trap:
               writedebug("red stack trap!\n")
               memory[0] = uint16(k.R[7])
               memory[1] = prev
               vec = 4
               panic("fatal")
           case nil:
               break
           default:
               panic(t)
           }
   */
    uint16_t prev = PS;
    switchmode(0);
    push(prev);
    push(R[7]);

    R[7] = dd11::read16(vec);
    PS = dd11::read16(vec + 2);
    if (prevuser)
    {
        PS |= (1 << 13) | (1 << 12);
    }
    waiting = false;
}

void interrupt(uint8_t vec, uint8_t pri)
{
    if (vec & 1)
    {
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% Thou darst calling interrupt() with an odd vector number?"));
        }
        panic();
    }
    // fast path
    if (itab[0].vec == 0)
    {
        itab[0].vec = vec;
        itab[0].pri = pri;
        return;
    }
    uint8_t i;
    for (i = 0; i < ITABN; i++)
    {
        if ((itab[i].vec == 0) || (itab[i].pri < pri))
        {
            break;
        }
    }
    for (; i < ITABN; i++)
    {
        if ((itab[i].vec == 0) || (itab[i].vec >= vec))
        {
            break;
        }
    }
    if (i >= ITABN)
    {
        if (PRINTSIMLINES)
        {
            _printf("%%%% interrupt table full (%i of %i)", i, ITABN);
        }
        panic();
    }
    uint8_t j;
    for (j = i + 1; j < ITABN; j++)
    {
        itab[j] = itab[j - 1];
    }
    itab[i].vec = vec;
    itab[i].pri = pri;
}

// pop the top interrupt off the itab.
static void popirq()
{
    uint8_t i;
    for (i = 0; i < ITABN - 1; i++)
    {
        itab[i] = itab[i + 1];
    }
    itab[ITABN - 1].vec = 0;
    itab[ITABN - 1].pri = 0;
}

void handleinterrupt()
{
    uint8_t vec = itab[0].vec;
    if (DEBUG_INTER)
    {
        if (PRINTSIMLINES)
        {
            Serial.print("%% IRQ: ");
            Serial.println(vec, OCT);
        }
    }
    uint16_t vv = setjmp(trapbuf);
    if (vv == 0)
    {
        uint16_t prev = PS;
        switchmode(0);
        push(prev);
        push(R[7]);
    }
    else
    {
        trapat(vv);
    }

    R[7] = dd11::read16(vec);
    PS = dd11::read16(vec + 2);
    if (prevuser)
    {
        PS |= (1 << 13) | (1 << 12);
    }
    waiting = false;
    popirq();
}
};  // namespace kb11

#endif
