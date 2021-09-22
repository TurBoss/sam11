// sam11 software emulation of DEC PDP-11/40 KT11 Memory Management Unit (MMU)

#include "kt11.h"

#include "kd11.h"
#include "platform.h"
#include "sam11.h"

#include <Arduino.h>

namespace kt11 {

uint16_t SLR;

struct page {
    uint16_t par;  // Page address register
    uint16_t pdr;  // Page descriptor register
    uint32_t addr()
    {
        return par & 07777;  // only bits 11-0 are valid
    }
    uint16_t len()
    {
        return ((pdr >> 8) & 0x7F);  // page length field - bits 14-8
    }
    bool read()
    {
        return (pdr & 2) == 2;
    }
    bool write()
    {
        return (pdr & 6) == 6;
    }
    bool ed()  // expansion directo, 0=up, 1=down
    {
        return (pdr & 8) == 8;
    }
};

page pages[16];
uint16_t SR0, SR2;

void errorSR0(const uint16_t a, const bool user)
{
    SR0 |= (a >> 12) & ~1;  // page no.
    if (user)
    {
        SR0 |= (1 << 5) | (1 << 6);  // user mode
    }
    SR2 = kd11::curPC;
}

uint32_t decode(const uint16_t a, const bool w, const bool user)
{
    if (!(SR0 & 1))
    {
        if (a >= 0170000)
            return (uint32_t)((uint32_t)a + 0600000);
        return a;
    }

    //if (kd11::isReg(a))
    //    return (uint32_t)((uint32_t)a + 0600000);

    // kt11 enabled
    const uint16_t m = user ? 8 : 0;
    const uint16_t i = (a >> 13) + m;
    const uint16_t block = (a >> 6) & 0177;
    const uint16_t disp = a & 077;

    if (w && !pages[i].write())  // write to RO page
    {
        SR0 = (1 << 13) | 1;  // abort RO
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            Serial.print(F("%% kt11::decode write to read-only page "));
            Serial.println(a, OCT);
        }
        longjmp(trapbuf, INTFAULT);
    }
    if (!pages[i].read())  // read from WO page
    {
        SR0 = (1 << 15) | 1;  //abort non-resident
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            Serial.print(F("%% kt11::decode read from no-access page "));
            Serial.println(a, OCT);
        }
        longjmp(trapbuf, INTFAULT);
    }
    if (pages[i].ed() && (block < pages[i].len()))
    {
        SR0 = (1 << 14) | 1;  //abort page len error
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            _printf("%%%% page %i length exceeded (down).\r\n", i);
            _printf("%%%% address 0%06o; block 0%03o is below length 0%03o\r\n", a, block, (pages[i].len()));
        }
        longjmp(trapbuf, INTFAULT);
    }
    if (!pages[i].ed() && block > pages[i].len())
    {
        SR0 = (1 << 14) | 1;  //abort page len error
        errorSR0(a, user);
        if (PRINTSIMLINES)
        {
            _printf("%%%% page %i length exceeded (up).\r\n", i);
            _printf("%%%% address 0%06o; block 0%03o is above length 0%03o\r\n", a, block, (pages[i].len()));
        }
        longjmp(trapbuf, INTFAULT);
    }

    if (w)
        pages[i].pdr |= 1 << 6;

    uint32_t aa = ((block + pages[i].addr()) << 6) + disp;

#if DEBUG_MMU
    if (DEBUG_MMU)
    {
        if (PRINTSIMLINES)
        {
            Serial.print("%% decode: slow ");
            Serial.print(a, OCT);
            Serial.print(" -> ");
            Serial.println(aa, OCT);
        }
    }
#endif

    return aa;
}

uint16_t read16(const uint32_t a)
{
    uint8_t i = ((a & 017) >> 1);
    if ((a >= 0772300) && (a < 0772320))
    {
        return pages[i].pdr;
    }
    if ((a >= 0772340) && (a < 0772360))
    {
        return pages[i].par;
    }
    if ((a >= 0777600) && (a < 0777620))
    {
        return pages[i + 8].pdr;
    }
    if ((a >= 0777640) && (a < 0777660))
    {
        return pages[i + 8].par;
    }
    if (PRINTSIMLINES)
    {
        Serial.print(F("%% kt11::read16 invalid read from "));
        Serial.println(a, OCT);
    }
    longjmp(trapbuf, INTBUS);
}

void write16(const uint32_t a, const uint16_t v)
{
    uint8_t i = ((a & 017) >> 1);
    if ((a >= 0772300) && (a < 0772320))
    {
        pages[i].pdr = v;
        pages[i].pdr &= ~(1 << 6);
        return;
    }
    if ((a >= 0772340) && (a < 0772360))
    {
        pages[i].par = v;
        pages[i].pdr &= ~(1 << 6);
        return;
    }
    if ((a >= 0777600) && (a < 0777620))
    {
        pages[i + 8].pdr = v;
        pages[i].pdr &= ~(1 << 6);
        return;
    }
    if ((a >= 0777640) && (a < 0777660))
    {
        pages[i + 8].par = v;
        pages[i].pdr &= ~(1 << 6);
        return;
    }
    if (PRINTSIMLINES)
    {
        Serial.print(F("%% kt11::write16 0"));
        Serial.print(v, OCT);
        Serial.print(F(" from invalid address 0"));
        Serial.println(a, OCT);
    }
    longjmp(trapbuf, INTBUS);
}

};  // namespace kt11
