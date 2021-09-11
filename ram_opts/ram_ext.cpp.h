// this file is inserted into ms11.cpp when extended ram is selected as the option

#ifndef RAM_OPT
#define RAM_OPT

// memory as words
int* intptr = reinterpret_cast<int*>(RAM_PTR_ADDR);
// memory as bytes
char* charptr = reinterpret_cast<char*>(RAM_PTR_ADDR);

void begin()
{
    // Xmem test
    xmem::SelfTestResults results;
    xmem::begin(false);
    results = xmem::selfTest();
    if (!results.succeeded)
    {
        Serial.println(F("xram test failure"));
        panic();
    }

    return;
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
    if (a < MAX_RAM_ADDRESS)
    {
        // change this to a memory device rather than swap banks
        xmem::setMemoryBank(bank(a), false);
        charptr[(a & 0x7fff)] = v & 0xff;
        return;
    }
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return;
}

void write16(uint32_t a, uint16_t v)
{
    if (a < MAX_RAM_ADDRESS)
    {
        // change this to a memory device rather than swap banks
        xmem::setMemoryBank(bank(a), false);
        intptr[(a & 0x7fff) >> 1] = v;
        return;
    }
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return;
}

uint16_t read16(uint32_t a)
{
    if (a < DEV_MEMORY)
    {
        // change this to a memory device rather than swap banks
        xmem::setMemoryBank(bank(a), false);
        return intptr[(a & 0x7fff) >> 1];
        return 0;
    }
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return 0;
}

#endif