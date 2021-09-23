
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

// sam11 software emulation of DEC PDP-11/40  MS11-MB Silicon Memory
// 256KiB/128KiW, but system can only access up to 248KiB/124KiW
#include "pdp1140.h"
#include "platform.h"
#include <SDFat.h>

/* MS11 Silicon Memory:
 * ====================
 * 
 * The MS11 series of boards hold the memory of the PDP-11/40 processor, the 
 * main system RAM.
 * The MS11 replaces earlier MM and KM series boards which held ferrite cores,
 * and implements the same address space in silicon.
 * 
 * The early boards were simply silicon RAM, but later versions added a small
 * MMU which was used to do error correction, route addresses to different
 * parts of the board, calculate parity, and remove the use of non-functioning
 * chips.
 *
 */

namespace ms11 {
#if RAM_MODE == RAM_SWAPFILE
extern SdFile msdata;
#endif
void begin();
void clear();
uint16_t read8(uint32_t a);
void write8(uint32_t a, uint16_t v);
void write16(uint32_t a, uint16_t v);
uint16_t read16(uint32_t a);
};  // namespace ms11
