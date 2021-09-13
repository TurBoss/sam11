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

#if USE_SDIO
SdFatSdio sd;
#else
SdFat sd;
#endif

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

    // init the sd card
    if (!sd.begin(PIN_OUT_SD_CS, SD_SCK_MHZ(7)))
        sd.initErrorHalt();

    // Load RK05 Disk 0 as Read/Write
    if (!rk11::rkdata.open("unixv6.dsk", O_RDWR))
    {
        sd.errorHalt("%% opening unixv6.dsk for write failed");
    }

    ky11::reset();  // reset the front panel - sets the switches to INST_BOOTSTRAP (0173030)
    kd11::reset();  // reset the processor
    Serial.println(F("%% Ready"));
}

static void loop0()
{
    while (1)
    {
        // Check for interruptd
        if ((itab[0].vec) && (itab[0].pri >= ((kd11::PS >> 5) & 7)))
        {
            kd11::handleinterrupt();
            return;  // reset the loop to reset interrupt
        }

#ifdef PIN_OUT_PROC_STEP
        digitalWrite(PIN_OUT_PROC_STEP, LED_ON);
#endif

        kd11::step();  // step the instructions

#ifdef PIN_OUT_PROC_STEP
        digitalWrite(PIN_OUT_PROC_STEP, LED_OFF);
#endif

        kw11::tick();  // tick the clock

        kl11::poll();  // check the terminal
    }
}

jmp_buf trapbuf;

void loop()
{
    // reset interrupts/trap vectors
    uint16_t vec = setjmp(trapbuf);
    if (vec)
    {
        kd11::trapat(vec);
    }
    loop0();  // restart step loop
}

void panic()  // aka what it does when halted
{
#ifdef PIN_OUT_PROC_RUN
    digitalWrite(PIN_OUT_PROC_RUN, LED_OFF);
#endif

    printstate();
    while (1)
        delay(1);
}
