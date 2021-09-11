// sam11 software emulation of DEC DD11 UNIBUS Backplane

#include "dd11.h"

#include "kd11.h"
#include "kl11.h"
#include "kt11.h"
#include "kw11.h"
#include "ky11.h"
#include "ms11.h"
#include "rk11.h"
#include "sam11.h"
#include "xmem.h"

#include <SdFat.h>

namespace dd11 {

uint16_t read8(const uint32_t a)
{
    if (a & 1)
    {
        return read16(a & ~1) >> 8;
    }
    return read16(a & ~1) & 0xFF;
}

void write8(const uint32_t a, const uint16_t v)
{
    if (a < DEV_MEMORY)
    {
        ms11::write8(a, v);
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
        Serial.print(F("%% dd11: write16 to odd address "));
        Serial.println(a, OCT);
        longjmp(trapbuf, INTBUS);
    }
    if (a < DEV_MEMORY)
    {
        ms11::write16(a, v);
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
            Serial.println(F("%% invalid mode"));
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
            Serial.println(F("%% invalid mode"));
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
    case DEV_CONSOLE_DR:
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
    Serial.print(F("%% dd11: write to invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
}

uint16_t read16(uint32_t a)
{
    if (a & 1)
    {
        Serial.print(F("%% dd11: read16 from odd address "));
        Serial.println(a, OCT);
        longjmp(trapbuf, INTBUS);
    }

    if (a < DEV_MEMORY)  // if lower than the device memory, then this is just RAM
    {
        ms11::read16(a);
        return;
    }

    switch (a)  // Switch by address, and read from virtual device as appropriate
    {
    case DEV_CPU_STAT:
        return kd11::PS;
    //case DEV_STACK_LIM:
    //    return (uint16_t)(0400); // probs wrong
    case DEV_LKS:
        return kw11::LKS;
    case DEV_MMU_SR0:
        return kt11::SR0;
    case DEV_MMU_SR2:
        return kt11::SR2;
    case DEV_CONSOLE_SR:
        return ky11::read16(a);
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

    Serial.print(F("%% dd11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
}

};  // namespace dd11
