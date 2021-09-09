// sam11 software emulation of DEC PDP-11/40 KY11 Front Panel

#include "ky11.h"

#include "sam11.h"

namespace ky11 {

extern uint16_t SR;
extern uint16_t DR;

void reset()
{
    SR = 0173030;
    DR = 0000000;
}

uint16_t read16(uint32_t addr)
{
    if (addr == DEV_CONSOLE_SW)
        return 0173030;  //SR;
    // read front panel switches
    return 0;
}
void write16(uint32_t a, uint16_t v)
{
    if (a == DEV_CONSOLE_SW)
        DR = v;
    // write to a front panel here
}
};  // namespace ky11
