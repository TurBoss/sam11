#include "platform.h"

#include "pdp1140.h"

#include <Arduino.h>

namespace platform {

// init the platform hardware
void begin()
{
#ifndef PIN_OUT_SD_CS
#if !USE_SDIOS
#error NO WAY TO USE SD CARD -> CANNOT COMPILE
#endif
#endif
    pinMode(PIN_OUT_SD_CS, OUTPUT);  // SD card chip select
    digitalWrite(PIN_OUT_SD_CS, HIGH);

#ifdef PIN_OUT_DISK_ACT
    pinMode(PIN_OUT_DISK_ACT, OUTPUT);  // rk11 disk led
    digitalWrite(PIN_OUT_DISK_ACT, LED_OFF);
#endif

#ifdef PIN_OUT_PROC_STEP
    pinMode(PIN_OUT_PROC_STEP, OUTPUT);  // timing interrupt, high while CPU is stepping
    digitalWrite(PIN_OUT_PROC_STEP, LED_OFF);
#endif

#ifdef PIN_OUT_BUS_ACT
    pinMode(PIN_OUT_BUS_ACT, OUTPUT);  // Bus in use
    digitalWrite(PIN_OUT_BUS_ACT, LED_OFF);
#endif

#ifdef PIN_OUT_PROC_RUN
    pinMode(PIN_OUT_PROC_RUN, OUTPUT);  // Processor running
    digitalWrite(PIN_OUT_PROC_RUN, LED_OFF);
#endif

    return;
}

void writeAddr(uint32_t addr) { }     // write to address pins
void writeData(uint16_t data) { }     // write to data pins
void writeDispReg(uint16_t disp) { }  // write display register
uint16_t readSwitches()               // read processor front switches
{
    return INST_BOOTSTRAP;
}
};  // namespace platform
