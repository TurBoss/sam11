#include "pc11.h"

#if USE_PC

namespace pc11 {
uint16_t read16(uint32_t a)
{
    switch (a)
    {
    case DEV_PC_RS:
        return RS;
    case DEV_PC_RB:
        RS = RS & ~0x80;  // Clear DONE,
        return RB;
    case DEV_PC_PS:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% pc11 read16: punch not implemented"));
        }
        return PS;
    case DEV_PC_PB:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% pc11 read16: punch not implemented"));
        }
        return PB;
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% pc11 read16: invalid read"));
        }
        // panic();
    }
    return 0;
}
};  // namespace pc11

#endif
