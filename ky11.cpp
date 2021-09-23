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

// sam11 software emulation of DEC PDP-11/40 KY11 Front Panel

#include "ky11.h"

#include "platform.h"
#include "sam11.h"

namespace ky11 {

uint16_t SR;   // data switches (not address or option switches!)
uint16_t DR;   // display register (separate to address/data displays)
uint16_t SLR;  // Register of status LEDs separate to addr/data/display

void reset()
{
    SR = INST_UNIX_SINGLEUSER;
    DR = 0000000;

    SR = platform::readSwitches();
}

uint16_t read16(uint32_t addr)
{
    // read front panel switches here
    SR = platform::readSwitches();

    if (addr == DEV_CONSOLE_SR)
        return SR;  //SR;
    return 0;
}
void write16(uint32_t a, uint16_t v)
{
    if (a == DEV_CONSOLE_DR)
        DR = v;

    // write to a front panel LEDs here
    platform::writeAddr(a);
    platform::writeData(v);
    platform::writeDispReg(DR);
}
};  // namespace ky11
