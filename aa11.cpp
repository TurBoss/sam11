// sam11 software emulation of DEC PDP-11/40 KL11 Main TTY

#include "aa11.h"

#include "kd11.h"
#include "sam11.h"

#include <Arduino.h>

namespace aa11 {

uint16_t TKS;
uint16_t TKB;
uint16_t TPS;
uint16_t TPB;

void clearterminal()
{
    TKS = 0;
    TPS = 1 << 7;
    TKB = 0;
    TPB = 0;
}

static void addchar(char c)
{
    switch (c)
    {
    case 42:
        TKB = 4;
        break;
    case 19:
        TKB = 034;
        break;
        //case 46:
        //	TKB = 127;
    default:
        TKB = c;
        break;
    }
    TKS |= 0x80;
    if (TKS & (1 << 6))
    {
        kd11::interrupt(INTTTYIN, 4);
    }
}

uint8_t count;

void poll()
{
    if (Serial.available())
    {
        addchar(Serial.read());
    }

    if ((TPS & 0x80) == 0)
    {
        if (++count > 32)
        {
            Serial.write(TPB & 0x7f);
            TPS |= 0x80;
            if (TPS & (1 << 6))
            {
                kd11::interrupt(INTTTYOUT, 4);
            }
        }
    }
}

uint16_t read16(uint32_t a)
{
    switch (a)
    {
    case 0776440:
        return TKS;
    case 0776442:
        if (TKS & 0x80)
        {
            TKS &= 0xff7e;
            return TKB;
        }
        return 0;
    case 0776444:
        return TPS;
    case 0776446:
        return 0;
    default:
        Serial.println(F("%% kl11: read16 from invalid address"));  // " + ostr(a, 6))
        panic();
        return 0;
    }
}

void write16(uint32_t a, uint16_t v)
{
    switch (a)
    {
    case 0776440:
        if (v & (1 << 6))
        {
            TKS |= 1 << 6;
        }
        else
        {
            TKS &= ~(1 << 6);
        }
        break;
    case 0776444:
        if (v & (1 << 6))
        {
            TPS |= 1 << 6;
        }
        else
        {
            TPS &= ~(1 << 6);
        }
        break;
    case 0776446:
        TPB = v & 0xff;
        TPS &= 0xff7f;
        count = 0;
        break;
    default:
        Serial.println(F("%% kl11: write16 to invalid address"));  // " + ostr(a, 6))
        panic();
    }
}

};  // namespace aa11
