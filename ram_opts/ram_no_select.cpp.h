// this file is inserted into ms11.cpp when extended ram is selected as the option

#ifndef RAM_OPT
#define RAM_OPT

void begin()
{
    // nothing to do
    return;
}

uint16_t read8(uint32_t a)
{
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return 0;
}

void write8(const uint32_t a, const uint16_t v)
{
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
}

void write16(uint32_t a, uint16_t v)
{
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return;
}

uint16_t read16(uint32_t a)
{
    Serial.print(F("%% ms11: read from invalid address "));
    Serial.println(a, OCT);
    longjmp(trapbuf, INTBUS);
    return 0;
}

#endif