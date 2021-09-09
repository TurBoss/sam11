<<<<<<< HEAD
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
=======
// sam11 software emulation of DEC PDP-11/40 KW11 Line Clock

namespace kw11 {

extern uint16_t LKS;
void reset();
void tick();
};  // namespace kw11
>>>>>>> c7d3598a5769d693dc7a598f78245d8118bd6ceb
