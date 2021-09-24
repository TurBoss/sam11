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
 * The real PDP-11 had no clock speed as it was all async, the main limit
 * For processor speed is memory and bus speed. That is also true here.
 * 
 * Table of devices/controllers:
 * =============================
 * 
 * Items marked 'Y' are for most purposes fully implemented
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
 * KB11-A   Y   Main CPU 11/45 \_ Main processor is mostly 11/40 
 * KD11-A   Y   Main CPU 11/40 /  but has a few 11/45 things
 * KE11-E   *   Extended instructions (EIS)
 * KG11     *   XOR/CRC "cagey" calculations controller
 * KJ11     +   Stack Limit Register
 * KL11     Y   Main Console TTY Interface
 * KM11         Maintenance Device
 * KT11     Y   Memory Management Unit (11/40 compliant ONLY)
 * KW11     *   Line Time Clock (P revision is also RTC - not implemented)
 * KY11-D   +   Developer/Diagnostics Console (front panel)
 * 
 * KE11-F   P*  Floating Point Instructions Extension
 * FP11     P*  Floating Point Coprocessor
 *  
 * Coms/Bus:
 *
 * AA11         (alias for DL11 type AA?)
 * BB11         (alias for DL11 type BB?)
 * DC11         Serial (async) Line Controller
 * DD11     Y   UNIBUS Backplane
 * DH11         Serial (async) Line Controller 
 * DJ11         Serial (async) Line Controller
 * DL11         Serial (async) Line Controller <- this is the one you add to expand the no. TTYs
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
 * KF11-A       Processor Core Memory (ignored if external memory exists)
 * MM11         Ferrite Core Memory
 * MS11     P+  Silicon Memory
 * 
 * Storage:
 *  
 * RK11     Y+  RK Hard Disk Controller (RK05) (only supports 1 disk currently)
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
    INTRFU = 0000,     // Reserved
    INTBUS = 0004,     // Bus timeout and generally other CPU/System faults
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
    INTTTYIN = 0060,   // TTY In ("Console Reader/Keyboard")
    INTTTYOUT = 0064,  // TTY Out ("Console Puncher/Printer")
    INTPCRD = 0070,    // PC11 Reader
    INTPCPU = 0074,    // PC11 Puncher
    INTCLOCK = 0100,   // Line CLock
    INTRTC = 0104,     // Programmer RTC
    INTXYP = 0120,     // XY Plotter
    INTDR = 0124,      // DR11B parallel interface
    INTADO = 0130,     // a/d subsystem
    INTAFC = 0134,     // Analogue control subsystem
    INTAAS = 0140,     // AA11 Scope
    INTAAL = 0144,     // AA11 Light
    // 150 - 164 not used
    INTUR0 = 0170,     // User Reserved
    INTUR1 = 0174,     // User Reserved
    INTLP = 0200,      // Line Printer
    INTRF = 0204,      // RF11 Disk Control
    INTRC = 0210,      // RC11 Disk Control
    INTTC = 0214,      // TC11 Disk Control
    INTRK = 0220,      // RK11 Disk Control
    INTTM = 0224,      // TM11 Disk Control
    INTCR = 0230,      // CR11 Disk Control
    INTUDC = 0234,     // Digital control subsystem
    INTPIRQ = 0240,    // 11/45 PIRQ
    INTFPUERR = 0244,  // FPU Error
    INTMMUERR = 0250,  // MMU Error
    INTRP = 0254,      // RP Disk Control
    INTTA = 0260,      // TA11 Cassette Control
    INTRX = 0264,      // RX11 Floppy Control
    INTUR2 = 0270,     // User Reserved
    INTUR3 = 0274,     // User Reserved
    INTFLOAT = 0300,   // Start of floating vectors
};

// Switch Settings:
enum
{
    INST_UNIX_SINGLEUSER = 0173030,  // this boots Unix into single user mode and keeps it there
};

// Device/Register Addresses:
enum
{
    DEV_CPU_STAT = 0777776,       // CPU Status
    DEV_STACK_LIM = 0777774,      // Stack Limit Register
    DEV_PIRQ = 0777772,           // Program Interrupt Request
    DEV_MICROPROG_BRK = 0777770,  // Microprogram break

    DEV_CPU_ERROR = 0777766,    // CPU Errors
    DEV_SYS_I_D = 0777764,      // System I/D
    DEV_SYS_SIZE_UP = 0777762,  // System Size Upper
    DEV_SYS_SIZE_LO = 0777760,  // System Size Lower

    // 0777756 - NOT USED
    // 0777754 - NOT USED

    DEV_HIT_MISS = 0777752,  // Hit/Miss Register
    DEV_MAINTAIN = 0777750,  // System Maintenance Register

    DEV_CONTROL = 0777746,        // Control Register
    DEV_MEM_SYS_ERROR = 0777744,  // Memory System Error
    DEV_ERROR_ADR_HI = 0777742,   // Error Address High
    DEV_ERROR_ADR_LO = 0777740,   // Error Address Low

    DEV_CPU_USR_SP = 0777717,   // CPU User Stack Pointer (R6)
    DEV_CPU_SUP_SP = 0777716,   // CPU Super Stack Pointer (R6)
    DEV_CPU_GEN1_R5 = 0777715,  // CPU General Register (set 1) R5
    DEV_CPU_GEN1_R4 = 0777714,  // CPU General Register (set 1) R4
    DEV_CPU_GEN1_R3 = 0777713,  // CPU General Register (set 1) R3
    DEV_CPU_GEN1_R2 = 0777712,  // CPU General Register (set 1) R2
    DEV_CPU_GEN1_R1 = 0777711,  // CPU General Register (set 1) R1
    DEV_CPU_GEN1_R0 = 0777710,  // CPU General Register (set 1) R0

    DEV_CPU_KER_PC = 0777707,   // CPU (Kernel) Program Counter (R7)
    DEV_CPU_KER_SP = 0777706,   // CPU Kernel Stack Pointer (R6)
    DEV_CPU_GEN0_R5 = 0777705,  // CPU General Register (set 0) R5
    DEV_CPU_GEN0_R4 = 0777704,  // CPU General Register (set 0) R4
    DEV_CPU_GEN0_R3 = 0777703,  // CPU General Register (set 0) R3
    DEV_CPU_GEN0_R2 = 0777702,  // CPU General Register (set 0) R2
    DEV_CPU_GEN0_R1 = 0777701,  // CPU General Register (set 0) R1
    DEV_CPU_GEN0_R0 = 0777700,  // CPU General Register (set 0) R0

    DEV_USR_DAT_PAR_R7 = 0777676,  // MMU User Data PAR Register 7
    DEV_USR_DAT_PAR_R6 = 0777674,  // MMU User Data PAR Register 6
    DEV_USR_DAT_PAR_R5 = 0777672,  // MMU User Data PAR Register 5
    DEV_USR_DAT_PAR_R4 = 0777670,  // MMU User Data PAR Register 4
    DEV_USR_DAT_PAR_R3 = 0777666,  // MMU User Data PAR Register 3
    DEV_USR_DAT_PAR_R2 = 0777664,  // MMU User Data PAR Register 2
    DEV_USR_DAT_PAR_R1 = 0777662,  // MMU User Data PAR Register 1
    DEV_USR_DAT_PAR_R0 = 0777660,  // MMU User Data PAR Register 0

    DEV_USR_INS_PAR_R7 = 0777656,  // MMU User Instruction PAR Register 7
    DEV_USR_INS_PAR_R6 = 0777654,  // MMU User Instruction PAR Register 6
    DEV_USR_INS_PAR_R5 = 0777652,  // MMU User Instruction PAR Register 5
    DEV_USR_INS_PAR_R4 = 0777650,  // MMU User Instruction PAR Register 4
    DEV_USR_INS_PAR_R3 = 0777646,  // MMU User Instruction PAR Register 3
    DEV_USR_INS_PAR_R2 = 0777644,  // MMU User Instruction PAR Register 2
    DEV_USR_INS_PAR_R1 = 0777642,  // MMU User Instruction PAR Register 1
    DEV_USR_INS_PAR_R0 = 0777640,  // MMU User Instruction PAR Register 0

    DEV_USR_DAT_PDR_R7 = 0777636,  // MMU User Data PDR Register 7
    DEV_USR_DAT_PDR_R6 = 0777634,  // MMU User Data PDR Register 6
    DEV_USR_DAT_PDR_R5 = 0777632,  // MMU User Data PDR Register 5
    DEV_USR_DAT_PDR_R4 = 0777630,  // MMU User Data PDR Register 4
    DEV_USR_DAT_PDR_R3 = 0777626,  // MMU User Data PDR Register 3
    DEV_USR_DAT_PDR_R2 = 0777624,  // MMU User Data PDR Register 2
    DEV_USR_DAT_PDR_R1 = 0777622,  // MMU User Data PDR Register 1
    DEV_USR_DAT_PDR_R0 = 0777620,  // MMU User Data PDR Register 0

    DEV_USR_INS_PDR_R7 = 0777616,  // MMU User Instruction PDR Register 7
    DEV_USR_INS_PDR_R6 = 0777614,  // MMU User Instruction PDR Register 6
    DEV_USR_INS_PDR_R5 = 0777612,  // MMU User Instruction PDR Register 5
    DEV_USR_INS_PDR_R4 = 0777610,  // MMU User Instruction PDR Register 4
    DEV_USR_INS_PDR_R3 = 0777606,  // MMU User Instruction PDR Register 3
    DEV_USR_INS_PDR_R2 = 0777604,  // MMU User Instruction PDR Register 2
    DEV_USR_INS_PDR_R1 = 0777602,  // MMU User Instruction PDR Register 1
    DEV_USR_INS_PDR_R0 = 0777600,  // MMU User Instruction PDR Register 0

    DEV_MMU_SR2 = 0777576,  // MMU System Register 2
    DEV_MMU_SR1 = 0777574,  // MMU System Register 1
    DEV_MMU_SR0 = 0777572,  // MMU System Register 0

    DEV_CONSOLE_SR = 0777570,  // Console switch/display register
    DEV_CONSOLE_DR = 0777570,  // Console display register

    DEV_LKS = 0777546,  // Line Clock

    DEV_RK_DB = 0777416,  // RK11 Data Buffer
    DEV_RK_DA = 0777412,  // RK11 Disk Address
    DEV_RK_BA = 0777410,  // RK11 Bus Address (current memory address)
    DEV_RK_WC = 0777406,  // RK11 Word Count
    DEV_RK_CS = 0777404,  // RK11 Control Status
    DEV_RK_ER = 0777402,  // RK11 Error
    DEV_RK_DS = 0777400,  // RK11 Drive Status

    DEV_KWP = 0772546,       // KW11-P (XX)
    DEV_KWP_CNTR = 0772544,  // KW11-P Counter
    DEV_KWP_CSB = 0772542,   // KW11-P Count Set Register
    DEV_KWP_CSR = 0772540,   // KW11-P CSR

    DEV_MEMORY = 0760000,  // Main Memory (0->0760000 (excl))
};
#endif
