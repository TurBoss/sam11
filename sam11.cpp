#include "sam11.h"

#include "dd11.h"
#include "kd11.h"
#include "kl11.h"
#include "kw11.h"
#include "ky11.h"
#include "rk11.h"
#include "pdp1140.h"
#include "xmem.h"

#include <Arduino.h>
#include <SdFat.h>

int serialWrite(char c, FILE* f)
{
    Serial.write(c);
    return 0;
}

SdFat sd;

void setup(void)
{
    // setup all the SPI pins, ensure all the devices are deselected
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);  // rk11
    pinMode(53, OUTPUT);
    digitalWrite(53, HIGH);
    pinMode(18, OUTPUT);
    digitalWrite(18, LOW);  // timing interrupt, high while CPU is stepping

    // Start the UART
    Serial.begin(115200);
    //fdevopen(serialWrite, NULL);

    Serial.println(F("Reset"));

    // Xmem test
    xmem::SelfTestResults results;

    xmem::begin(false);
    results = xmem::selfTest();
    if (!results.succeeded)
    {
        Serial.println(F("xram test failure"));
        panic();
    }

    // Initialize SdFat or print a detailed error message and halt
    // Use half speed like the native library.
    // change to SPI_FULL_SPEED for more performance.
    if (!sd.begin(4, SD_SCK_MHZ(7)))
        sd.initErrorHalt();

    if (!rk11::rkdata.open("unixv6.rk0", O_RDWR))
    {
        sd.errorHalt("opening unixv6.rk0 for write failed");
    }

    ky11::reset();
    kd11::reset();
    Serial.println(F("Ready"));
}

// On a 16Mhz atmega 2560 this loop costs 21usec per emulated instruction
// This cost is just the cost of the loop and fetching the instruction at the PC.
// Actual emulation of the instruction is another ~40 usec per instruction.
static void loop0()
{
    for (;;)
    {
        //the itab check is very cheap
        if ((itab[0].vec) && (itab[0].pri >= ((kd11::PS >> 5) & 7)))
        {
            kd11::handleinterrupt();
            return;  // exit from loop to reset trapbuf
        }

        digitalWrite(18, HIGH);
        kd11::step();
        digitalWrite(18, LOW);

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
    printstate();
    for (;;)
        delay(1);
}
