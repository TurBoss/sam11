#include <stdint.h>

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
 * Table of devices/controllers:
 * =============================
 * 
 * Items marked 'Y' are fully implemented
 * Items marked 'P' are partially implemented
 * Items marked '*' are implemented as part of another module
 * Items marked '+' are work in progress
 * 
 * This list is not exhaustive, and there will be controllers not listed.
 * 
 * Num:     I:  Description:
 * ---------------------------------------------------------------------------- 
 * 
 * Processor and Extensions:
 * 
 * KB11-A   Y   Main CPU 11/45 \_ Main processor is a hybrid of the pair
 * KD11-A   Y   Main CPU 11/40 /
 * KE11-E   *   Extended instructions (EIS)
 * KG11     *   XOR/CRC "cagey" calculations controller
 * KJ11     +   Stack Limit Register
 * KL11     Y   Main TTY Interface
 * KM11         Maintenance Device
 * KT11     P   Memory Management Unit (MMU)
 * KW11     *   Line Time Clock (P revision is also RTC)
 * KY11     +   Developer/Diagnostics Console (front panel)
 * 
 * KE11-F   P*  Floating Point Instructions Extension
 * FP11     P*  Floating Point Coprocessor
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
 * DR11         Parallel Controller
 * DS11         Serial (sync) Line Controller 
 * DT11         Bus Switch
 * DU11         Serial (sync) Line Controller 
 * DZ11         Serial (async) Line Controller 
 *  
 * Memory (18-bit address = 248KiB Max):
 *  
 * MM11         Ferrite Core Memory
 * MS11     P   Silicon Memory
 * 
 * Storage:
 *  
 * RK11     Y   RK Hard Disk Controller (RK05)
 * RF11         RS Disk Controller
 * RL11         RL Disk Controller
 * RP11         RP Disk Pack Controller
 * RC11         RS Disk Controller
 * PC11         PC Punch Tape Controller
 * TC11         TU DECtape Controller (TU56)
 * TM11         TU/TE Magnetic Tape Controller (TU10)
 * CR11         CR/CM Card Controller
 * RX211        RX Floppy Disk Controller 
 * 
 * Printers:
 * 
 * LP11         Line Printer
 * 
*/

#ifndef _H_PDP1140_
#define _H_PDP1140_

enum
{
    ENABLE_LKS = true,
};

namespace pdp11 {

struct intr {
    uint8_t vec;
    uint8_t pri;
};
};  // namespace pdp11

// Vectors & Addresses from DEC PDP-11/40 Processor Handbook Appendix B (1972)

// Interrupt Vectors:
enum
{
    INTRFU = 0000,       // Reserved
    INTBUS = 0004,       // Bus timeout
    INTINVAL = 0010,     // Reserved Intruction
    INTDEBUG = 0014,     // Debugging trap
    INTIOT = 0020,       // IOT Trap
    INTPF = 0024,        // Power Fail
    INTEMT = 0030,       // EMT
    INTTRP = 0034,       // "TRAP"
    INTSS0 = 0040,       // System Software 0
    INTSS1 = 0044,       // System Software 1
    INTSS2 = 0050,       // System Software 2
    INTSS3 = 0054,       // System Software 3
    INTTTYIN = 0060,     // TTY In
    INTTTYOUT = 0064,    // TTY Out
    INTPCRD = 0070,      // PC11 Reader
    INTPCPU = 0074,      // PC11 Puncher
    INTCLOCK = 0100,     // Line CLock
    INTRTC = 0104,       // Programmer RTC
    INTXYP = 0120,       // XY Plotter
    INTDR = 0124,        // DR11B parallel interface
    INTADO = 0130,       //
    INTAFC = 0134,       //
    INTAAS = 0140,       // AA11 Scope
    INTAAL = 0144,       // AA11 Light
    INTUR0 = 0170,       // User Reserved
    INTUR1 = 0174,       // User Reserved
    INTLP = 0200,        // Line Printer
    INTRF = 0204,        // RF11 Disk Control
    INTRC = 0210,        // RC11 Disk Control
    INTTC = 0214,        // TC11 Disk Control
    INTRK = 0220,        // RK11 Disk Control
    INTTM = 0224,        // TM11 Disk Control
    INTCR = 0230,        // CR11 Disk Control
    INTUDC = 0234,       //
    INTPIRQ1145 = 0240,  // 11/45 PIRQ
    INTFPUERR = 0244,    // FPU Error
    INTFAULT = 0250,     // General Fault (not in DEC's manual....?)
    INTRP = 0254,        // RP Disk Control
};

// Device Addresses:
enum
{
    DEV_CPU_STAT = 0777776,   // CPU Status
    DEV_STACK_LIM = 0777774,  // Stack Limit Register
    DEV_PIRQ_1145 = 0777772,  // 11/45 PIRQ Register
    // 0777716 to 0777700,// CPU Registers
    // 0777676 to 0777600 // 11/45 Segmentation Registers
    // 0777656 to 0777600 // MX11 Memory Extention
    DEV_MMU_SR2 = 0777576,
    DEV_MMU_SR0 = 0777572,
    DEV_CONSOLE_SW = 0777570,  // Console switch/display register
    DEV_LKS = 0777546,
    DEV_MEMORY = 0760000,  // Main Memory (0->0760000 (excl))
};
#endif
