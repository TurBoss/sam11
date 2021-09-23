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

// sam11 software emulation of DEC PDP-11/40 KL11 Main TTY
#include "pdp1140.h"

namespace kl11 {

enum
{
    BAUD_100 = 100,    // REV E
    BAUD_110 = 110,    // REV A
    BAUD_150 = 150,    // REV B
    BAUD_300 = 300,    // REV C
    BAUD_600 = 600,    // REV D
    BAUD_1200 = 1200,  // REV E
    BAUD_2400 = 2400,  // REV F
    BAUD_DEFAULT = BAUD_1200,
};

void write16(uint32_t a, uint16_t v);
uint16_t read16(uint32_t a);
void clearterminal();
void poll();

};  // namespace kl11
