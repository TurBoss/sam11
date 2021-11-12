#if !H_CPU_BUS
#define H_CPU_BUS 1

static uint16_t read8(const uint16_t a)
{
    return dd11::read8(kt11::decode_instr(a, false, curuser));
}

static uint16_t read16(const uint16_t a)
{
    return dd11::read16(kt11::decode_instr(a, false, curuser));
}

static void write8(const uint16_t a, const uint16_t v)
{
    dd11::write8(kt11::decode_instr(a, true, curuser), v);
}

static void write16(const uint16_t a, const uint16_t v)
{
    dd11::write16(kt11::decode_instr(a, true, curuser), v);
}

static uint16_t memread16(const uint16_t a)
{
    if (isReg(a))
    {
        return R[a & 7];
    }
    return read16(a);
}

static uint16_t memread(uint16_t a, uint8_t l)
{
    if (isReg(a))
    {
        const uint8_t r = a & 7;
        if (l == 2)
        {
            return R[r];
        }
        else
        {
            return R[r] & 0xFF;
        }
    }
    if (l == 2)
    {
        return read16(a);
    }
    return read8(a);
}

static void memwrite16(const uint16_t a, const uint16_t v)
{
    if (isReg(a))
    {
        R[a & 7] = v;
    }
    else
    {
        write16(a, v);
    }
}

static void memwrite(const uint16_t a, const uint8_t l, const uint16_t v)
{
    if (isReg(a))
    {
        const uint8_t r = a & 7;
        if (l == 2)
        {
            R[r] = v;
        }
        else
        {
            R[r] &= 0xFF00;
            R[r] |= v;
        }
        return;
    }
    if (l == 2)
    {
        write16(a, v);
    }
    else
    {
        write8(a, v);
    }
}

static uint16_t fetch16()
{
    const uint16_t val = read16(R[7]);
    R[7] += 2;
    return val;
}

#endif