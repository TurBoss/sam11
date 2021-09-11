// this file is inserted into ms11.cpp when extended ram is selected as the option

#ifndef RAM_OPT
#define RAM_OPT

// memory as words
int* intptr = reinterpret_cast<int*>(RAM_PTR_ADDR);
// memory as bytes
char* charptr = reinterpret_cast<char*>(RAM_PTR_ADDR);

void begin()
{
    return;
}

void write8(const uint32_t a, const uint16_t v)
{
    if (a < MAX_RAM_ADDRESS)
    {
        charptr[a] = v & 0xff;
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
        intptr[a >> 1] = v;
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
        return intptr[a >> 1];
    }
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return 0;
}

#endif