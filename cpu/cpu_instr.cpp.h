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

#if !H_CPU_INSTR
#define H_CPU_INSTR 1

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
    PS |= (curuser << 14);
    PS |= (prevuser << 12);
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
    kl11::reset();
    rk11::reset();
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

static bool _HALT()
{
    // if (curuser)  // modded, usually a HALT in user mode fails with a trap
    // {
    //     return true;
    // }
    //Serial.println(F("%% HALT"));
    panic();
    return false;
}

static bool _WAIT()
{
    if (curuser)
    {
        return true;
    }
    waiting = true;
    return false;
}

#endif