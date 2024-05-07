
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

// sam11 software emulation of DEC PDP-11/40  MS11-MB Silicon Memory
// 256KiB/128KiW, but system can only access up to 248KiB/124KiW
#include "pdp1140.h"
#include "platform.h"
#include <SdFat.h>

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
