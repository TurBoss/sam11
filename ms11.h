// sam11 software emulation of DEC PDP-11/40  MS11-MB Silicon Memory
// 256KiB/128KiW, but system can only access up to 248KiB/124KiW
#include "pdp1140.h"

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
void begin();
uint16_t read8(uint32_t a);
void write8(uint32_t a, uint16_t v);
void write16(uint32_t a, uint16_t v);
uint16_t read16(uint32_t a);
};  // namespace ms11
