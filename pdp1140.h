/*
 * The PDP-11 series of processors use hardware and software drivers with part
 * numbers with the pattern Letter.Letter.11 E.g. DD11 or RX11
 * 
 * This code follows the original numbering scheme, with the different files
 * running the emulators for different parts of the emulated hardware.
 * 
 * "sam11.*" is the exception to this as it's the main glue of the emulator and
 * its code doesn't directly emulate a part of the hardware.
 * 
 * The structure of the data flow, is more-or-less identical to the real PDP-11
 * 
 * CPU -> Backplane -> Device -> Backplane -> CPU
 *  
 * 
 * Table of devices:
 * =================
 * 
 * Num:     I:  Description:
 * ---------------------------------------------------------------------------- 
 * 
 * Core Processor:
 *  
 * KD11     Y   Main CPU
 * KE11         Extended instructions (KE11-E for ext, KE11-F for floats)
 * KG11         XOR/CRC "cagey" calculations controller
 * KJ11     P   Stack Limit Register
 * KL11     Y   Main TTY Interface
 * KM11         Maintanance Device
 * KT11     Y   Memory Management Unit (MMU)
 * KW11     Y   Line Time Clock (P revision is also RTC)
 * KY11         Developer/Diagnostics Console (front panel)
 * FP11         Floating Point Coprocessor
 *  
 * Coms/Bus:
 *
 * AA11         (alias for DM11 type AA)
 * BB11         (alias for DM11 type BB)
 * DC11         Serial (async) Line Controller
 * DD11     Y   UNIBUS Backplane
 * DH11         Serial (async) Line Controller 
 * DJ11         Serial (async) Line Controller
 * DL11         Serial (async) Line Controller
 * DM11         Serial (async) Line Controller
 * DQ11         Serial (NPR sync) Line Controller  
 * DS11         Serial (sync) Line Controller 
 * DT11         Bus Switch
 * DU11         Serial (sync) Line Controller 
 * DZ11         Serial (async) Line Controller 
 *  
 * Memory (248Ki Words Max):
 *  
 * MM11         Ferrite Core Memory
 * MS11     Y   Silicon Memory
 * 
 * Storage:
 *  
 * RK11     Y   RK Hard Disk Controller
 * RF11         RS Disk Controller
 * RL11         RL Disk Controller
 * RP11         RP Disk Pack Controller
 * RC11         RS Disk Controller
 * PC11         PC Punch Tape Controller
 * TC11         TU DECtape Controller (TU56)
 * TM11         TU/TE Magnetic Tape Controller (TU10)
 * CR11         CR/CM Card Controller
 * RX211        RF Floppy Disk Controller 
 * 
 * Printers:
 * 
 * LP11         Line Printer
 * 
*/

// Interrupt Vectors:
enum
{
    INTRFU = 0000,     // Reserved
    INTBUS = 0004,     // Bus timeout
    INTINVAL = 0010,   // Reserved Intruction
    INTDEBUG = 0014,   // Debugging trap
    INTIOT = 0020,     // IOT Trap
    INTPF = 0024,      // Power Fail
    INTEMT = 0030,     // EMT
    INTTRP = 0034,     // "TRAP"
    INTSS0 = 0040,     // System Software 0
    INTSS1 = 0044,     // System Software 1
    INTSS2 = 0050,     // System Software 2
    INTSS3 = 0054,     // System Software 3
    INTTTYIN = 0060,   // TTY In
    INTTTYOUT = 0064,  // TTY Out
    INTPCRD = 0070,    // PC11 Reader
    INTPCPU = 0074,    // PC11 Puncher
    INTCLOCK = 0100,   // Line CLock
    INTRTC = 0104,     // Programmer RTC
    INTXYPLOT = 0120,  // XY Plotter
    INTDR = 0124,      // DR11B parallel interface
    INTADO1 = 0130,    //
    INTAFC = 0134,     //
    INTAASCP = 0140,   //
    INTAALAMP = 0144,  //
    INTUR0 = 0170,     //
    INTUR1 = 0174,     //
    INTLP = 0200,      // Line Printer
    INTRF = 0204,      // RF11 Disk Control
    INTRC 0210,        // RC11 Disk Control
    INTTC11 = 0214,    // TC11 Disk Control
    INTRK = 0220,      // RK11 Disk Control
    INTTM = 0224,      // TM11 Disk Control
    INTCR = 0230,      // CR11 Disk Control
    INTUDC = 0234,     //
    INT45PIRQ = 0240,  // 11/45 PIRQ
    INTFPUERR = 0244,  // FPU Error
    INTFAULT = 0250,   //
    INTRP = 0254,      // RP Disk Control
};

// Device Addresses:
#define BUS_CPU_STAT  0777776  // CPU Status
#define BUS_STACK_LIM 0777774  // Stack Limit Register
#define BUS_MEMORY    0760000  // Main Memory (0->0760000)
