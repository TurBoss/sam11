// sam11 software emulation of DEC PDP-11/40 KL11 Main TTY
#include "pdp1140.h"

namespace aa11 {

enum
{
    BAUD_100 = 100,    // REV E
    BAUD_110 = 110,    // REV A
    BAUD_150 = 150,    // REV B
    BAUD_300 = 300,    // REV C
    BAUD_600 = 600,    // REV D
    BAUD_1200 = 1200,  // REV E
    BAUD_2400 = 2400,  // REV F
    BAUD_DEFAULT = BAUD_1200,
};

void write16(uint32_t a, uint16_t v);
uint16_t read16(uint32_t a);
void clearterminal();
void poll();

};  // namespace aa11
