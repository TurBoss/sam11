// sam11 software emulation of DEC PDP-11/40 KY11 Front Panel

#include "ky11.h"

#include "sam11.h"

namespace ky11 {

uint16_t SR;  // data switches (not address or option switches!)
uint16_t DR;  // display register (separate to address/data displays)

void reset()
{
    SR = 0173030;
    DR = 0000000;
}

uint16_t read16(uint32_t addr)
{
    // read front panel switches here

    if (addr == DEV_CONSOLE_SR)
        return 0173030;  //SR;
    return 0;
}
void write16(uint32_t a, uint16_t v)
{
    if (a == DEV_CONSOLE_DR)
        DR = v;

    // write to a front panel LEDs here
}
};  // namespace ky11
