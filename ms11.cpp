/*
MIT License

Copyright (c) 2021 Chloe Lunn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

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
