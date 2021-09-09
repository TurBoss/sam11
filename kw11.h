// sam11 software emulation of DEC PDP-11/40 KW11 Line Clock
#include "pdp1140.h"

namespace kw11 {

extern uint16_t LKS;
void reset();
void tick();
};  // namespace kw11
