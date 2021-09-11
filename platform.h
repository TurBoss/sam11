#include "pdp1140.h"

#ifndef H_PLATFORM
#define H_PLATFORM

#define RAM_SPI      (0)  // Use SPI SRAM chips (not implemented)
#define RAM_INTERNAL (1)  // Use internal RAM addresses
#define RAM_EXTENDED (2)  // Use extended, internal RAM (xmem library for AVRs)
#define RAM_PARALLEL (3)  // Use Parallel addr/data RAM chips (not implmented)

namespace platform {

void begin();
void writeAddr(uint32_t addr);
void writeData(uint16_t data);
void writeDispReg(uint16_t disp);
uint16_t readSwitches();

// Arduino Mega and similar
#if defined(__AVR_ATmega2560__)

#define ALLOW_DISASM    (true)
#define MAX_RAM_ADDRESS (0760000)  // 248KB

// See ATmega 2560 datasheet section 9.1
#define RAM_MODE     RAM_EXTENDED
#define RAM_PTR_ADDR (0x2200)  // where in memory is the RAM -> this is external to the main RAM addresses in AVRs

#define LED_ON  (LOW)
#define LED_OFF (HIGH)

#define PIN_OUT_DISK_ACT  (13)
#define PIN_OUT_SD_CS     (4)
#define PIN_OUT_PROC_STEP (18)
#define PIN_OUT_PROC_RUN  (0)
#define PIN_OUT_BUS_ACT   (0)

// Adafruit Grand Central M4 and similar
#elif defined(__SAMD51P20A__)

#define ALLOW_DISASM    (false)
#define MAX_RAM_ADDRESS (0760000)  // 248KB

#define RAM_MODE     RAM_INTERNAL
#define RAM_PTR_ADDR (0x20000000 + 0x2000)  // SRAM base + 8K buffer for simulator to run in

#define LED_ON  (LOW)
#define LED_OFF (HIGH)

#define PIN_OUT_DISK_ACT  (13)
#define PIN_OUT_SD_CS     (4)
#define PIN_OUT_PROC_STEP (18)
#define PIN_OUT_PROC_RUN  (0)
#define PIN_OUT_BUS_ACT   (0)

#endif

};  // namespace platform

#endif