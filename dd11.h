// sam11 software emulation of DEC DD11 UNIBUS Backplane
#include "pdp1140.h"

/* DD11 - DEC UNIBUS Backplane - "MUD"
 * =================================
 * 
 * If the KD11 processor is the heart of a PDP 11/40 then the DD11 Backplane
 * is the skeleton. The backplane held all the cards that made up the entire
 * PDP 11/40 processor, device controller cards, and peripheral devices.
 * 
 * The DD11 was one of the simpler versions of backplane, offering up to 9
 * slots with the "standard" 18-bit address, 16-bit data size.
 * 
 * The UNIBUS standard was a simple asynchronous bus, with separate, parallel
 * address and data lines, along with power routing, control/handshake lines
 * and interrupts.
 * 
 * For speed, the dd11 software unit does not route any control lines, but
 * instead the units that require control or interrupts call the bus or cpu
 * units directly in a first-come-first-serve order (due to the single thread
 * linear structure of the software). 
 * 
 * As the PDP-11/40 does not have a system processor clock in the way that we 
 * would understand it currently, the processor execution speed relies on the 
 * speed and timeout of the UNIBUS.
 * 
 * Whilst there is some speed drop due to the emulated instructions, in general
 * this is also true here, with slow emulated devices slowing down processor 
 * execution speed. Specifically, SRAM and SD Card read/write speeds.
 * 
 * Each PDP-11 processor that used UNIBUS had a fixed "UNIBUS Map". This Map
 * was in fact just a list of fixed addresses where compatible controllers,
 * registers, and device cards would "sit", and if not present the bus would
 * simply fire the timeout interrupt. 
 * 
 * e.g. The RK11 disk controller always sits at address 0777400 on an 18-bit
 * address system
 * 
 * Later processors and UNIBUS standards with 22-bits worked in a similar way,
 * but with prepended addresses of 017, e.g. 017777400 for the RK11.
 * 
 * Due to the limit of 18-bit address lines, the 11/40 cpu can only access up
 * to 256KB of space, this includes all the device addresses on the UNIBUS.
 * Actual system memory sits at addresses 0->0760000, thus only 248KB.
 * 
 */

namespace dd11 {

// operations on uint32_t types are insanely expensive
union addr {
    uint8_t bytes[4];
    uint32_t value;
};

uint16_t read8(uint32_t addr);
uint16_t read16(uint32_t addr);
void write8(uint32_t a, uint16_t v);
void write16(uint32_t a, uint16_t v);
};  // namespace dd11
