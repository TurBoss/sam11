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

// sam11 software emulation of DEC PDP-11/40 KW11 Line Clock

#include "kw11.h"

#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "pdp1140.h"
#include "platform.h"

#ifndef LKS_ACC
#define LKS_ACC LKS_SHIFT_TICK
#endif

#if LKS_ACC == LKS_MID_ACC || LKS_ACC == LKS_HIGH_ACC
#include <elapsedMillis.h>
#endif

#if USE_11_45
#define procNS kb11
#else
#define procNS kd11
#endif

namespace kw11 {

uint16_t LKS;

#if LKS_ACC == LKS_SHIFT_TICK
uint16_t time;
uint32_t multip = 1000;
#define LKS_PER = LKS_PERIOD_MS
#elif LKS_ACC == LKS_MID_ACC
elapsedMillis time;
uint16_t multip = 1;
#define LKS_PER LKS_PERIOD_MS
#elif LKS_ACC == LKS_HIGH_ACC
elapsedMicros time;
uint32_t multip = 1;
#define LKS_PER LKS_PERIOD_US
#endif

void reset()
{
    LKS = 1 << 7;
    time = 0;
}
void tick()
{
    bool tick = false;

#if LKS_ACC == LKS_SHIFT_TICK
    ++time;
    //tick = (time >> 8 == 1 << 6);  //0b0100000000000000
#endif

    tick = !!(time >= (LKS_PER * multip));

    if (tick)
    {
        time = 0;

        LKS |= (1 << 7);
        if (LKS & (1 << 6))
        {
            procNS::interrupt(INTCLOCK, 6);
        }
    }
}
};  // namespace kw11
