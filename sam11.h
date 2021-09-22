#include "pdp1140.h"

#ifndef H_SAM11
#define H_SAM11

enum
{
    PRINTINSTR = false,
    PRINTSTATE = false,
    INSTR_TIMING = false,
    DEBUG_INTER = false,
    DEBUG_TRAP = false,
    DEBUG_RK05 = false,
    DEBUG_MMU = false,
    BREAK_ON_TRAP = false,
    FIRST_LF_BREAKS = false,
    PRINTSIMLINES = false,
};

void printstate();
void panic();
void disasm(uint32_t ia);

void trap(uint16_t num);

#endif
