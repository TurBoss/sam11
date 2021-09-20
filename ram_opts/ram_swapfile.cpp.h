// this file is inserted into ms11.cpp when extended ram is selected as the option

#ifndef RAM_OPT
#define RAM_OPT

void begin()
{
#ifdef PIN_OUT_MEM_ACT
    digitalWrite(PIN_OUT_MEM_ACT, LED_ON);
#endif
    // open the swap file as R/W
    if (!msdata.open("swapfile", O_RDWR))
    {
        Serial.println(F("%% swapfile: failed to open for write"));
        panic();
    }
#ifdef PIN_OUT_MEM_ACT
    digitalWrite(PIN_OUT_MEM_ACT, LED_OFF);
#endif
    return;
}

uint16_t read8(const uint32_t a)
{
#ifdef PIN_OUT_MEM_ACT
    digitalWrite(PIN_OUT_MEM_ACT, LED_ON);
#endif
    int val;
    if (!msdata.seekSet(a))
    {
        Serial.println(F("%% swapfile: failed to seek"));
        panic();
    }
    int t = msdata.read();
    if (t == -1)
    {
        Serial.println(F("%% swapfile: failed to read"));
        panic();
    }
    val = t & 0xFF;
#ifdef PIN_OUT_MEM_ACT
    digitalWrite(PIN_OUT_MEM_ACT, LED_OFF);
#endif
    return val;
}

void write8(const uint32_t a, const uint16_t v)
{
#ifdef PIN_OUT_MEM_ACT
    digitalWrite(PIN_OUT_MEM_ACT, LED_ON);
#endif
    if (!msdata.seekSet(a))
    {
        Serial.println(F("%% swapfile: failed to seek"));
        panic();
    }
    msdata.write(v & 0xFF);
#ifdef PIN_OUT_MEM_ACT
    digitalWrite(PIN_OUT_MEM_ACT, LED_OFF);
#endif
    return;
}

void write16(uint32_t a, uint16_t v)
{
#ifdef PIN_OUT_MEM_ACT
    digitalWrite(PIN_OUT_MEM_ACT, LED_ON);
#endif
    if (!msdata.seekSet(a))
    {
        Serial.println(F("%% swapfile: failed to seek"));
        panic();
    }
    msdata.write(v & 0xFF);
    msdata.write((v >> 8) & 0xFF);
#ifdef PIN_OUT_MEM_ACT
    digitalWrite(PIN_OUT_MEM_ACT, LED_OFF);
#endif
    return;
}

uint16_t read16(uint32_t a)
{
#ifdef PIN_OUT_MEM_ACT
    digitalWrite(PIN_OUT_MEM_ACT, LED_ON);
#endif
    int val;
    if (!msdata.seekSet(a))
    {
        Serial.println(F("%% swapfile: failed to seek"));
        panic();
    }
    int t = msdata.read();
    if (t == -1)
    {
        Serial.println(F("%% swapfile: failed to read (low)"));
        panic();
    }
    val = t & 0xFF;

    t = msdata.read();
    if (t == -1)
    {
        Serial.println(F("%% swapfile: failed to read (high)"));
        panic();
    }
    val += ((t & 0xFF) << 8);
#ifdef PIN_OUT_MEM_ACT
    digitalWrite(PIN_OUT_MEM_ACT, LED_OFF);
#endif
    return val;
}

#endif
