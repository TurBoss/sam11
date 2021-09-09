<<<<<<< HEAD
#include "kd11.h"
#include "kw11.h"

=======
// sam11 software emulation of DEC PDP-11/40 KW11 Line Clock

#include "kw11.h"

#include "kd11.h"

>>>>>>> c7d3598a5769d693dc7a598f78245d8118bd6ceb
namespace kw11 {

uint16_t LKS;

<<<<<<< HEAD
=======
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
>>>>>>> c7d3598a5769d693dc7a598f78245d8118bd6ceb
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
<<<<<<< HEAD
                cpu::interrupt(INTCLOCK, 6);
=======
                kd11::interrupt(INTCLOCK, 6);
>>>>>>> c7d3598a5769d693dc7a598f78245d8118bd6ceb
            }
        }
    }
}
<<<<<<< HEAD

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
=======
};  // namespace kw11
>>>>>>> c7d3598a5769d693dc7a598f78245d8118bd6ceb
