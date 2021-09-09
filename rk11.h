// sam11 software emulation of DEC PDP-11/40 RK11 RK Disk Controller

namespace rk11 {

extern SdFile rkdata;

void reset();
void write16(uint32_t a, uint16_t v);
uint16_t read16(uint32_t a);
};  // namespace rk11

enum
{
    RKOVR = (1 << 14),
    RKNXD = (1 << 7),
    RKNXC = (1 << 6),
    RKNXS = (1 << 5)
<<<<<<< HEAD
};
=======
};
>>>>>>> c7d3598a5769d693dc7a598f78245d8118bd6ceb
