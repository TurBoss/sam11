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

#include "kd11.h"
#include "pdp1140.h"

namespace kw11 {

uint16_t LKS;

union {
    struct {
        uint8_t low;
        uint8_t high;
    } bytes;
    uint16_t value;
} clkcounter;

void reset()
{
    LKS = 1 << 7;
    clkcounter.value = 0;
}
void tick()
{
    if (ENABLE_LKS)
    {
        ++clkcounter.value;
        if (clkcounter.bytes.high == 1 << 6)
        {
            clkcounter.value = 0;
            LKS |= (1 << 7);
            if (LKS & (1 << 6))
            {
                kd11::interrupt(INTCLOCK, 6);
            }
        }
    }
}
};  // namespace kw11
