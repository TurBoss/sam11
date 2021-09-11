// sam11 software emulation of DEC PDP-11/40  MS11-MB Silicon Memory
// 256KiB/128KiW, but system can only access up to 248KiB/124KiW

#include "pdp1140.h"

namespace ms11 {
void begin();
void write8(uint32_t a, uint16_t v);
void write16(uint32_t a, uint16_t v);
uint16_t read16(uint32_t a);
};  // namespace ms11
