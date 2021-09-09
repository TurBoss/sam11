// sam11 software emulation of DEC PDP-11/40 KL11 Main TTY
#include "pdp1140.h"

namespace kl11 {

void write16(uint32_t a, uint16_t v);
uint16_t read16(uint32_t a);
void clearterminal();
void poll();

};  // namespace kl11
