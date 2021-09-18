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

#if defined(__AVR_ATmega2560__)
int serialWrite(char c, FILE* f)
{
    Serial.write(c);
    return 0;
}
#endif

#if USE_SDIO
SdFatSdio sd;
#else
SdFat sd;
#endif

const char* disks[] =
  {
    "unixv6.dsk",
    // "unixv5.dsk",
    // "rsts.dsk",
    // "xxdp.dsk",
    // "rt11v3.dsk",
    // "rt11v4.dsk",
    // "rt11v5.dsk",
    // "dos-11.dsk",
    // "calderav6_0.dsk"
};

void setup(void)
{
    // init the board/platform
    platform::begin();

    // Start the UART
    Serial.begin(kl11::BAUD_DEFAULT);  // may open bootloader on some boards...

#if defined(__AVR_ATmega2560__)
    fdevopen(serialWrite, NULL);
#endif

    while (!Serial)
        ;

    // Serial.println("%% RK05 Boot List:");
    // Serial.println("%% ===================================================");
    // Serial.println("%%  0: UnixV6    - UnixV6 image from Ken");
    // Serial.println("%%  1: UnixV5    - UnixV5 image from Ken");
    // Serial.println("%%  2: RSTS      - RSTS DEC OS");
    // Serial.println("%%  3: XXDP      - DEC XXDP Diagnostics tool");
    // Serial.println("%%  4: RT11 v3   - RT11 v3 DOS");
    // Serial.println("%%  5: RT11 v4   - RT11 v4 DOS");
    // Serial.println("%%  6: RT11 v5   - RT11 v5 DOS");
    // Serial.println("%%  7: DOS-11v9  - DOS-11 v9");
    // Serial.println("%%  8: CalderaV6 - UnixV6 from Caldera Ancient Unix");  // This one is multi-part, we only load disk 0

    // Serial.println("\r\n%% Type disk number followed by '.' to select image.");

    char disk;
    char c;
    char cnt;
reprompt:
    disk = 0;
    cnt = 0;

    Serial.print("%% disk=");
    // while (!Serial.available())
    //     delay(1);
    // c = 0;
    // while (1)
    // {
    //     c = Serial.read();
    //     if (isprint((c)))
    //     {
    //         if (c >= '0' && c <= '9')
    //         {
    //             Serial.print(c);
    //             disk += (c - '0') + (disk * 10 * cnt);
    //             cnt++;
    //         }
    //     }
    //     if (c == '.')
    //         break;
    //     c = 0;
    // }
    // Serial.println();

    // if (disk > 8)
    //     goto reprompt;

    Serial.print(F("%% Init disk "));
    Serial.print(disk, DEC);
    Serial.print(", ");
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
    kd11::reset();  // reset the processor
    Serial.println(F("%% Ready\r\n"));
}

static void loop0()
{
    while (1)
    {
        //delayMicroseconds(10);  // a touch of throttle... the processor is plenty fast enough, so we add this to mimic the slower pdp

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

        Serial.flush();
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
    Serial.println("%% Processor halted.");

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
