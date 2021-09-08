#include "kd11.h"
#include "kw11.h"

namespace kw11 {

uint16_t LKS;

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
                cpu::interrupt(INTCLOCK, 6);
            }
        }
    }
}

void reset()
{
    LKS = 1 << 7;
}

void write16(uint32_t a, uint16_t v)
{
    if (a == 0777546)
        LKS = v;
}

uint16_t read16(uint32_t a)
{
    if (a == 0777546)
        return LKS;
}

};  // namespace kw11
