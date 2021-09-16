#include "pdp1140.h"

enum
{
    PRINTINSTR = false,
    PRINTSTATE = false,
    INSTR_TIMING = false,
    DEBUG_INTER = false,
    DEBUG_RK05 = false,
    DEBUG_MMU = false,
    BREAK_ON_TRAP = true,
    FIRST_LF_BREAKS = true,
};

void printstate();
void panic();
void disasm(uint32_t ia);

void trap(uint16_t num);
