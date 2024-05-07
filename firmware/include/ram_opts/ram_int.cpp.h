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
    return charptr[a];
}

void write8(const uint32_t a, const uint16_t v)
{
    charptr[a] = v & 0xff;
    return;
}

void write16(uint32_t a, uint16_t v)
{
    intptr[a >> 1] = v;
    return;
}

uint16_t read16(uint32_t a)
{
    return intptr[a >> 1];
}

#endif