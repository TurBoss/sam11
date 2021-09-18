// sam11 software emulation of DEC PDP-11/40 KT11 Memory Management Unit (MMU)
#include "pdp1140.h"

namespace kt11 {

extern uint16_t SLR;

extern uint16_t SR0;
extern uint16_t SR2;

uint32_t decode(uint16_t a, bool w, bool user);
uint16_t read16(uint32_t a);
void write16(uint32_t a, uint16_t v);

};  // namespace kt11
