// sam11 software emulation of DEC PDP-11/40 KY11 Front Panel

namespace ky11 {
extern uint16_t SR;
extern uint16_t DR;
void reset();
uint16_t read16(uint32_t addr);
void write16(uint32_t a, uint16_t v);
};  // namespace ky11
