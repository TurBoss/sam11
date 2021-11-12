#if !H_CPU_CORE
#define H_CPU_CORE 1

bool isReg(const uint16_t a)
{
    return (a & 0177770) == 0170000;
}

static void push(const uint16_t v)
{
    R[6] -= 2;
    write16(R[6], v);
}

static uint16_t pop()
{
    const uint16_t val = read16(R[6]);
    R[6] += 2;
    return val;
}

static void branch(int16_t o)
{
    if (o & 0x80)
    {
        o = -(((~o) + 1) & 0xFF);
    }
    o <<= 1;
    R[7] += o;
}

static bool N()
{
    return (uint8_t)PS & FLAGN;
}

static bool Z()
{
    return (uint8_t)PS & FLAGZ;
}

static bool V()
{
    return (uint8_t)PS & FLAGV;
}

static bool C()
{
    return (uint8_t)PS & FLAGC;
}

static void setZ(bool b)
{
    if (b)
        PS |= FLAGZ;
}

// aget resolves the operand to a vaddress.
// if the operand is a register, an address in
// the range [0170000,0170007). This address range is
// technically a valid IO page, but dd11 doesn't map
// any addresses here, so we can safely do this.
static uint16_t aget(uint8_t v, uint8_t l)
{
    uint16_t addr = 0;
    if (((v & 7) >= 6) || (v & 010))
    {
        l = 2;
    }
    if ((v & 070) == 000)
    {
        return 0170000 | (v & 7);
    }
    switch (v & 060)
    {
    case 000:
        v &= 07;
        addr = R[v & 07];
        break;
    case 020:
        addr = R[v & 07];
        R[v & 07] += l;
        break;
    case 040:
        R[v & 07] -= l;
        addr = R[v & 07];
        break;
    case 060:
        addr = fetch16();
        addr += R[v & 07];
        break;
    }
    //addr &= 0xFFFF;  // ?
    if (v & 010)
    {
        addr = read16(addr);
    }
    return addr;
}

#endif