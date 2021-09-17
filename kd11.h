// sam11 software emulation of DEC PDP-11/40 KD11-A processor
// Mostly 11/40 KD11-A with KE11/KG11 extensions from 11/45 KB11-B

// this is all kinds of wrong
#include <setjmp.h>
#include "pdp1140.h"

extern jmp_buf trapbuf;

#define ITABN 16

extern pdp11::intr itab[ITABN];

enum
{
    FLAGN = 8,
    FLAGZ = 4,
    FLAGV = 2,
    FLAGC = 1
};

namespace kd11 {

extern volatile int32_t R[8];  // R6 = SP, R7 = PC

extern volatile uint16_t curPC;  // R7
extern volatile uint16_t PS;     // Processor Status
extern volatile uint16_t USP;    // R6 (user)
extern volatile uint16_t KSP;    // R6 (kernel)
extern volatile bool curuser;    // (true = user)
extern volatile bool prevuser;   // (true = user)
extern volatile bool trapped;

void step();
void reset(void);
void switchmode(bool newm);

void trapat(uint16_t vec);
void interrupt(uint8_t vec, uint8_t pri);
void handleinterrupt();

static bool N()
{
    return (uint8_t)PS & FLAGN;
}

static bool Z()
{
    return (uint8_t)PS & FLAGZ;
}

static bool V()
{
    return (uint8_t)PS & FLAGV;
}

static bool C()
{
    return (uint8_t)PS & FLAGC;
}

};  // namespace kd11
