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

#if !H_CPU_IRQ
#define H_CPU_IRQ 1

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
            printstate();
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
    PS |= (curuser << 14);
    PS |= (prevuser << 12);
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
    PS |= (curuser << 14);
    PS |= (prevuser << 12);
    waiting = false;
    popirq();
}

#endif