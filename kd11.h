// sam11 software emulation of DEC PDP-11/40 KD11-A processor
// Mostly 11/40 KD11-A with KE11/KG11 extensions from 11/45 KB11-B

// this is all kinds of wrong
#include <setjmp.h>

extern jmp_buf trapbuf;

namespace pdp11 {
struct intr {
    uint8_t vec;
    uint8_t pri;
};
};  // namespace pdp11

#define ITABN 8

extern pdp11::intr itab[ITABN];

enum
{
    FLAGN = 8,
    FLAGZ = 4,
    FLAGV = 2,
    FLAGC = 1
};

namespace kd11 {

extern int32_t R[8];

extern uint16_t PC;
extern uint16_t PS;
extern uint16_t USP;
extern uint16_t KSP;
extern bool curuser;
extern bool prevuser;

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

<<<<<<< HEAD
};  // namespace cpu
=======
};  // namespace kd11
>>>>>>> c7d3598a5769d693dc7a598f78245d8118bd6ceb
