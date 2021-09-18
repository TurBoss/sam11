// this file is inserted into ms11.cpp when extended ram is selected as the option

#ifndef RAM_OPT
#define RAM_OPT

volatile char int_mem[MAX_RAM_ADDRESS];

// memory as words
#define intptr ((uint16_t*)&int_mem)
// memory as bytes
#define charptr ((char*)&int_mem)

void begin()
{
    // nothing to do
    return;
}

uint16_t read8(const uint32_t a)
{
    //if (a < MAX_RAM_ADDRESS)
    //{
    return charptr[a];
    //}
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return 0;
}

void write8(const uint32_t a, const uint16_t v)
{
    //if (a < MAX_RAM_ADDRESS)
    //{
    charptr[a] = v & 0xff;
    return;
    //}
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return;
}

void write16(uint32_t a, uint16_t v)
{
    //if (a < MAX_RAM_ADDRESS)
    //{
    //charptr[a] = (v >> 8) & 0xFF;
    //charptr[a + 1] = v & 0xFF;
    intptr[a >> 1] = v;
    return;
    //}
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return;
}

uint16_t read16(uint32_t a)
{
    //if (a < MAX_RAM_ADDRESS)
    //{
    //int16_t ret = 0;
    //ret = charptr[a] << 8;
    //ret += charptr[a + 1];
    return intptr[a >> 1];  //ret;
    //}
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return 0;
}

#endif