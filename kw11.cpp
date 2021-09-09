// sam11 software emulation of DEC PDP-11/40 KW11 Line Clock

#include "kw11.h"

#include "kd11.h"

namespace kw11 {

uint16_t LKS;

union {
    struct {
        uint8_t low;
        uint8_t high;
    } bytes;
    uint16_t value;
} clkcounter;
uint16_t instcounter;

void reset()
{
    LKS = 1 << 7;
}
void tick()
{
    if (ENABLE_LKS)
    {
        ++clkcounter.value;
        if (clkcounter.bytes.high == 1 << 6)
        {
            clkcounter.value = 0;
            LKS |= (1 << 7);
            if (LKS & (1 << 6))
            {
                kd11::interrupt(INTCLOCK, 6);
            }
        }
    }
}
};  // namespace kw11
