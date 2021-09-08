/*
 *  The PDP-11 series of processors use hardware and software drivers with part \
 *  numbers with the pattern Letter.Letter.11 E.g. DD11 or RX11
 *  
 *  This code follows the original numbering scheme, with the different files \
 *  running the emulators for different parts of the emulated hardware.
 *  
 *  "sam11.*" is the exception to this as it is the main glue of the emulator and \
 *  its code doesn't directly emulate a part of the hardware.
 *  
 *  Num:    I:  Description:
 *  
 *  Core Processor:
 *  
 *  KD11    Y   Main CPU
 *  KE11    N   Extended instructions (KE11-E for ext, KE11-F for floating point)
 *  KJ11    P   Stack Limit Register
 *  KL11    Y   Main TTY Interface
 *  KM11    N   Maintanance Device
 *  KT11    Y   Memory Management Unit (MMU)
 *  KW11    Y   Line Time Clock
 *  KY11    N   Developer/Diagnostics Console (front panel)
 *  
 *  Bus:
 *  
 *  DD11    Y   UNIBUS "Backplane" - Routes memory requests to the corrosponding virtual device code
 *  
 *  Memory (248Ki Words Max):
 *  
 *  MC11    N   Ferrite Core Memory
 *  MS11    Y   Silicon Memory
 *  
 *  Storage:
 *  
 *  RK11    Y   RK Hard Disk Controller (emulates an RK05)
 *  RF11    N   RF Floppy Disk Controller
 *  RL11    N   RL Mag Tape Controller
*/

// UNIBUS Addresses of different Hardware
#define KD11 0000000
#define MS11 0760000
