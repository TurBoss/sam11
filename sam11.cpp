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

const char* disks[] =
  {
    "unixv6.dsk",
    "unixv5.dsk",
    "rsts.dsk",
    "xxdp.dsk",
    "rt11v3.dsk",
    "rt11v4.dsk",
    "rt11v5.dsk",
    "dennisv6.dsk",
    "dos-11.dsk"};

void setup(void)
{
    // init the board/platform
    platform::begin();

    // Start the UART
    Serial.begin(kl11::BAUD_DEFAULT);
    //fdevopen(serialWrite, NULL);

    while (!Serial)
        ;

    // Serial.println("%% Boot List:");
    // Serial.println("%% ===================");
    // Serial.println("%% *0: UnixV6   (11/40)*");
    // Serial.println("%%  1: UnixV5   (11/45)");
    // Serial.println("%%  2: RSTS     (11/45)");
    // Serial.println("%%  3: XXDP     (11/45)");
    // Serial.println("%%  4: RT11 v3  (11/45)");
    // Serial.println("%%  5: RT11 v4  (11/45)");
    // Serial.println("%%  6: RT11 v5  (11/45)");
    // Serial.println("%%  7: RichieV6 (11/40)");
    // Serial.println("%%  8: DOS-11v9 (11/40)");

    // reprompt:
    //     Serial.print("%% ?:");

    // noprompt:

    char disk = 0;

    Serial.print(F("%% Init "));
    Serial.println(disks[disk]);

    // Initialise the RAM
    ms11::begin();

    // init the sd card
    if (!sd.begin(PIN_OUT_SD_CS, SD_SCK_MHZ(7)))
        sd.initErrorHalt();

    // Load RK05 Disk 0 as Read/Write
    if (!rk11::rkdata.open(disks[disk], O_RDWR))
    {
        sd.errorHalt("%% opening disk for write failed");
    }

    ky11::reset();  // reset the front panel - sets the switches to INST_BOOTSTRAP (0173030)

    Serial.println(F("%% Ready"));

    kd11::reset();  // reset the processor
}

static void loop0()
{
    while (1)
    {
        delayMicroseconds(1);  // a touch of throttle... the processor is plenty fast enough, so we add this to mimic the slower pdp

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
    delayMicroseconds(1);
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
    Serial.println("%% HALT");

#ifdef PIN_OUT_PROC_RUN
    digitalWrite(PIN_OUT_PROC_RUN, LED_OFF);
#endif

#ifdef PIN_OUT_PROC_STEP
    digitalWrite(PIN_OUT_PROC_STEP, LED_OFF);
#endif

    printstate();
    while (1)
        delay(1);
}
