namespace kw11 {

union {
    struct {
        uint8_t low;
        uint8_t high;
    } bytes;
    uint16_t value;
} clkcounter;

uint16_t instcounter;

extern uint16_t LKS;

void tick();
void reset();
void write16(uint32_t a, uint16_t v);
uint16_t read16(uint32_t a);

};  // namespace kw11
