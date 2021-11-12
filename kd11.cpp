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

// sam11 software emulation of DEC KD11-A processor
// Mostly 11/40 KD11-A with KE11/KG11 extensions from 11/45 KB11-B

#include "kd11.h"

#if !USE_11_45 || STRICT_11_40

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

namespace kd11 {

// signed integer registers -- change to R[2][8]
volatile int32_t R[8];  // R6 = SP, R7 = PC

volatile uint16_t PS;     // Processor Status
volatile uint16_t curPC;  // R7, address of current instruction
volatile uint16_t KSP;    // R6 (kernel), stack pointer
volatile uint16_t USP;    // R6 (user), stack pointer

volatile uint8_t curuser;   // (true = user)
volatile uint8_t prevuser;  // (true = user)

volatile bool trapped = false;
volatile bool cont_with = false;
volatile bool waiting = false;

#include "./cpu/cpu_bus.cpp.h"

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
    USP = 0;
    curuser = 0;
    prevuser = 0;
    kt11::SR0 = 0;
    kt11::SR1 = 0;
    kt11::SR2 = 0;
    kt11::SR3 = 0;
    curPC = 0;
    kw11::reset();
    ms11::clear();
    for (i = 0; i < BOOT_LEN; i++)
    {
        dd11::write16(BOOT_START + (i * 2), bootrom_rk0[i]);
    }
    R[7] = BOOT_START;
    kl11::reset();
    rk11::reset();
    waiting = false;

#ifdef PIN_OUT_PROC_RUN
    digitalWrite(PIN_OUT_PROC_RUN, LED_ON);
#endif
}

#include "./cpu/cpu_core.cpp.h"

void switchmode(uint8_t newm)
{
#if STRICT_11_40
    if (newm)
        newm = 3;
#endif

    prevuser = curuser;
    curuser = newm;
    if (prevuser == 3)
    {
        USP = R[6];
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

#include "./cpu/cpu_debug.cpp.h"
#include "./cpu/cpu_instr.cpp.h"

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
            if (prevuser == 3)
            {
                uval = USP;
            }
            // else if (prevuser == 1)
            // {
            //     uval = SSP;
            // }
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
        else if (prevuser == 3)
        {
            USP = uval;
        }
        // else if (prevuser == 1)
        // {
        //     USP = uval;
        // }
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

void step()
{
    if (waiting)
        return;

    debug_step();

    curPC = R[7];
    uint16_t instr = dd11::read16(kt11::decode_instr(R[7], false, curuser));
    // return;
    R[7] += 2;

    debug_print();

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
    if (((instr & 0177000) == 0104000) || (instr == 3) || (instr == 4))
    {  // EMT TRAP IOT BPT
        EMTX(instr);
        return;
    }
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
    case 00:  // HALT
        if (_HALT())
            break;
        return;
    case 01:  // WAIT
        if (_WAIT())
            break;
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
    }

#if USE_FIS
    // FIS
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
#endif

    if (PRINTSIMLINES)
    {
        Serial.print("%% invalid instruction 0");
        Serial.println(instr, OCT);
    }
    longjmp(trapbuf, INTINVAL);
}

#include "./cpu/cpu_irq.cpp.h"

};  // namespace kd11

#endif
