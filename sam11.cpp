#include "sam11.h"

#include "dd11.h"
#include "kd11.h"
#include "kl11.h"
#include "kw11.h"
#include "ky11.h"
#include "ms11.h"
#include "pdp1140.h"
#include "platform.h"
#include "rk11.h"
#include "xmem.h"

#include <Arduino.h>
#include <SdFat.h>

// int serialWrite(char c, FILE* f)
// {
//     Serial.write(c);
//     return 0;
// }

SdFat sd;

void setup(void)
{
    // init the board/platform
    platform::begin();

    // Start the UART
    Serial.begin(kl11::BAUD_DEFAULT);
    //fdevopen(serialWrite, NULL);

    Serial.println(F("%% Reset"));

    // Initialise the RAM
    ms11::begin();

    // Initialize SdFat or print a detailed error message and halt
    // Use half speed like the native library.
    // change to SPI_FULL_SPEED for more performance.
    if (!sd.begin(PIN_OUT_SD_CS, SD_SCK_MHZ(7)))
        sd.initErrorHalt();

    // Load RK05 Disk 0 as Read/Write
    if (!rk11::rkdata.open("unixv6.rk0", O_RDWR))
    {
        sd.errorHalt("%% opening unixv6.rk0 for write failed");
    }

    ky11::reset();  // reset the front panel - sets the switches to INST_BOOTSTRAP (0173030)
    kd11::reset();  // reset the processor
    Serial.println(F("%% Ready"));
}

// On a 16Mhz atmega 2560 this loop costs 21usec per emulated instruction
// This cost is just the cost of the loop and fetching the instruction at the PC.
// Actual emulation of the instruction is another ~40 usec per instruction.
static void loop0()
{
    while (1)
    {
        //the itab check is very cheap
        if ((itab[0].vec) && (itab[0].pri >= ((kd11::PS >> 5) & 7)))
        {
            kd11::handleinterrupt();
            return;  // exit from loop to reset trapbuf
        }

        digitalWrite(PIN_OUT_PROC_STEP, LED_ON);
        kd11::step();
        digitalWrite(PIN_OUT_PROC_STEP, LED_OFF);

        kw11::tick();

        // costs 3 usec
        kl11::poll();
    }
}

jmp_buf trapbuf;

void loop()
{
    uint16_t vec = setjmp(trapbuf);
    if (vec)
    {
        kd11::trapat(vec);
    }
    loop0();
}

void panic()
{
    digitalWrite(PIN_OUT_PROC_RUN, LED_OFF);
    printstate();
    while (1)
        delay(1);
}
