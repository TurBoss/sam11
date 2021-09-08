// sam11 software emulation of DEC DD11 UNIBUS Backplane

#include "dd11.h"

#include "kd11.h"
#include "kl11.h"
#include "kt11.h"
#include "kw11.h"
#include "ms11.h"
#include "rk11.h"
#include "sam11.h"
#include "xmem.h"

#include <SdFat.h>

namespace dd11 {

// memory as words
int* intptr = reinterpret_cast<int*>(0x2200);
// memory as bytes
char* charptr = reinterpret_cast<char*>(0x2200);

uint16_t read8(const uint32_t a)
{
    if (a & 1)
    {
        return read16(a & ~1) >> 8;
    }
    return read16(a & ~1) & 0xFF;
}

static uint8_t bank(const uint32_t a)
{
    // This shift costs 1 Khz of simulated performance,
    // at least 4 usec / instruction.
    // return a >> 15;
    char* aa = (char*)&a;
    return ((aa[2] & 3) << 1) | (((aa)[1] & (1 << 7)) >> 7);
}

void write8(const uint32_t a, const uint16_t v)
{
    if (a < DEV_MEMORY)
    {
        // change this to a memory device rather than swap banks
        xmem::setMemoryBank(bank(a), false);
        charptr[(a & 0x7fff)] = v & 0xff;
        return;
    }
    if (a & 1)
    {
        write16(a & ~1, (read16(a) & 0xFF) | (v & 0xFF) << 8);
    }
    else
    {
        write16(a & ~1, (read16(a) & 0xFF00) | (v & 0xFF));
    }
}

void write16(uint32_t a, uint16_t v)
{
    if (a % 1)
    {
        Serial.print(F("dd11: write16 to odd address "));
        Serial.println(a, OCT);
        longjmp(trapbuf, INTBUS);
    }
    if (a < DEV_MEMORY)
    {
        // change this to a memory device rather than swap banks
        xmem::setMemoryBank(bank(a), false);
        intptr[(a & 0x7fff) >> 1] = v;
        return;
    }
    switch (a)
    {
    case DEV_CPU_STAT:
        switch (v >> 14)
        {
        case 0:
            kd11::switchmode(false);
            break;
        case 3:
            kd11::switchmode(true);
            break;
        default:
            Serial.println(F("invalid mode"));
            panic();
        }
        switch ((v >> 12) & 3)
        {
        case 0:
            kd11::prevuser = false;
            break;
        case 3:
            kd11::prevuser = true;
            break;
        default:
            Serial.println(F("invalid mode"));
            panic();
        }
        kd11::PS = v;
        return;
    case DEV_LKS:
        kw11::LKS = v;
        return;
    case DEV_MMU_SR0:
        kt11::SR0 = v;
        return;
    case DEV_CONSOLE_SW:
        ky11::write16(a, v);
        return;
    }
    if ((a & 0777770) == 0777560)
    {
        kl11::write16(a, v);
        return;
    }
    if ((a & 0777700) == 0777400)
    {
        rk11::write16(a, v);
        return;
    }
    if (((a & 0777600) == 0772200) || ((a & 0777600) == 0777600))
    {
        kt11::write16(a, v);
        return;
    }
    Serial.print(F("dd11: write to invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
}

uint16_t read16(uint32_t a)
{
    if (a & 1)
    {
        Serial.print(F("dd11: read16 from odd address "));
        Serial.println(a, OCT);
        longjmp(trapbuf, INTBUS);
    }

    if (a < DEV_MEMORY)
    {
        // change this to a memory device rather than swap banks
        xmem::setMemoryBank(bank(a), false);
        return intptr[(a & 0x7fff) >> 1];
    }

    switch (a)
    {
    case DEV_CPU_STAT:
        return kd11::PS;
    case DEV_LKS:
        return kw11::LKS;
    case DEV_MMU_SR0:
        return kt11::SR0;
    case DEV_MMU_SR2:
        return kt11::SR2;
    case DEV_CONSOLE_SW:
        return ky11.read16(a);
    default:
        break;
    }

    if ((a & 0777770) == 0777560)
    {
        return kl11::read16(a);
    }

    if ((a & 0777760) == 0777400)
    {
        return rk11::read16(a);
    }

    if (((a & 0777600) == 0772200) || ((a & 0777600) == 0777600))
    {
        return kt11::read16(a);
    }

    Serial.print(F("dd11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
}

};  // namespace dd11
