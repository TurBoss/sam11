// sam11 software emulation of DEC PDP-11/40 MS11-MB Silicon Memory
// 256KiB/128KiW, but system can only access up to 248KiB/124KiW

#include "ms11.h"

#include "kd11.h"
#include "platform.h"
#include "sam11.h"

#include <Arduino.h>
#include "sam11.h"

#if RAM_MODE == RAM_EXTENDED
#include "xmem.h"
#endif

#if RAM_MODE == RAM_SWAPFILE
#include <SdFat.h>
#include <stdint.h>
#endif

namespace ms11 {
#if RAM_MODE == RAM_SWAPFILE
SdFile msdata;
#endif
void clear()
{
}
#if RAM_MODE == RAM_EXTENDED
#include "ram_opts/ram_ext.cpp.h"
#elif RAM_MODE == RAM_INTERNAL
#include "ram_opts/ram_int.cpp.h"
#elif RAM_MODE == RAM_SWAPFILE
#include "ram_opts/ram_swapfile.cpp.h"
#else
#include "ram_opts/ram_no_select.cpp.h"  // if there is no ram option, add some dummy functions
#error NO RAM OPTION SELECTED
#endif
};  // namespace ms11
