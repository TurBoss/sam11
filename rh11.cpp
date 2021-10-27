/*
Modified BSD License

Copyright (c) 2021 Chloe Lunn

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// sam11 software emulation of DEC PDP-11/40 RP11/RH11 RP/RM Disk Controller

#if USE_RP

#include "rh11.h"

#define NUM_RP_DRIVES (4)  // max 8!

#define NUM_WRD_P_SEC (256)     // Words per sector
#define RP_CTRL_ID    (0)       // Controlling RP drives
#define RM_CTRL_ID    (1)       // Controlling RM drives
#define MAX_XFER      (1 << 6)  // max transfer

// Other than for the RP03, the RP disk interface was actually called RH11, the UNIX driver is HP for these drives, and RP for those on the actual RP11 controller
namespace rh11 {

uint16_t RPCS1, RPWC, RPBA, RPCS2, RPDB, RPCS3;

// boatload of registers
uint16_t RPDA[NUM_RP_DRIVES];   // Track|Sector
uint16_t RPDS[NUM_RP_DRIVES];   // Status
uint16_t RPER1[NUM_RP_DRIVES];  // Error status 1
uint16_t RPLA[NUM_RP_DRIVES];   // Look ahead
uint16_t RPMR1[NUM_RP_DRIVES];  // Maintenance Reg 1
uint16_t RPMR2[NUM_RP_DRIVES];  // Maintenance Reg 2
uint16_t RPDT[NUM_RP_DRIVES];   //
uint16_t RPSN[NUM_RP_DRIVES];   //
uint16_t RPOF[NUM_RP_DRIVES];   //
uint16_t RPDC[NUM_RP_DRIVES];   // Cylinder
uint16_t RPCC[NUM_RP_DRIVES];   //
uint16_t RPER2[NUM_RP_DRIVES];  // Error Status 2
uint16_t RPER3[NUM_RP_DRIVES];  // Error Status 3
uint16_t RPEC1[NUM_RP_DRIVES];  // Error Correct 1
uint16_t RPEC2[NUM_RP_DRIVES];  // Error Correct 2

SdFile rpdata[NUM_TM_DRIVES];
uint16_t attached_drives[NUM_RP_DRIVES];  // doubles as DType register

uint16_t wordspsector[NUM_RP_DRIVES];
uint16_t sectors[NUM_RP_DRIVES];
uint16_t surfaces[NUM_RP_DRIVES];
uint16_t cylinders[NUM_RP_DRIVES];

// current position
uint16_t sector, syrface, cylinder, drive;

// Drive Type register
/*
 *   15  14  13  12  11  10  09  08  07  06  05  04  03  02  01  00
 *  ................................................................
 * |  0|  0| MOH| 0 |DPM| 0 | 0 | 0 |       Drive Type Code         |
 *  ................................................................
 * 
 * MOH = Moving Head
 * DPM drive opperating in Dual Port Mode
 * 
 */
enum  // Drive Type Register
{
    MOH = 020000,  // Moving Head
    DPM = 004000,  // Dual Port
    SI = 01,       // Sector interleave

    RP02 = -2,  // not actually part of the RH11 controller, so we will handle these differently.
    RP03 = -3,

    RS03 = 000,
    RS04 = 002,

    RP04 = MOH + 020,
    RP05 = MOH + 021,
    RP06 = MOH + 022,

    RM03 = MOH + 024,
    RM02 = MOH + 025,
    RM80 = MOH + 026,
    RM05 = MOH + 027,

    RP07 = MOH + 042,  // Note: the RP07, despite its designation, belongs to the RM family
};

enum
{
    NUMWD = 256,  // max no. words per sector
    RP = 0,
    RM = 1,
    NUM
};

int rpmaxblock(int drive)
{
    return cylinders[drive] * surfaces[drive] * sectors[drive];
}

static void rpnotready(int drive)
{
#ifdef PIN_OUT_DISK_ACT
    digitalWrite(PIN_OUT_DISK_ACT, LED_ON);
#endif

    RPDS[drive] &= ~0100000;
    RPCS1 &= ~0100000;

    RPDS[drive] &= ~(0200);
    RPCS1 &= ~(0200);
}

void rpready(int drive)
{
    RPDS[drive] |= 0100000;
    RPCS1 |= 0100000;

    RPDS[drive] |= 0200;
    RPCS1 |= 0200;

#ifdef PIN_OUT_DISK_ACT
    digitalWrite(PIN_OUT_DISK_ACT, LED_OFF);
#endif
}

// get size of a drive
int rpsize(int drive)
{
    if (!attached_drives[drive])
        return 0;

    return (rpmaxblock() * NUMWD);
}

// get position in image for disk
int rppos(int drive)
{
    return ((((RPDC[drive] * surfaces[drive]) + ((RPDA[drive] >> 8)) * sectors[drive]) + (RPDA[drive] & 0377)) * NUMWD);
    // return ((RPDC[drive]) * (RPDS[drive] >> 8) * (RPDA[drive] & 0377) * NUMWD);
}

int reset()
{
    RPCS1 = 0x880;
    RPCS2 = 0;
    RPAS = 0;
    RPWC = 0;
    RPCS3 = 0;
    RPBA = 0;

    for (int i = 0; i < NUM_RP_DRIVES; i++)
    {
        //     Port No.,  Firmware,    Serial Number
        RPSN = (i + 1) + (1 << 4) + (((uint8_t)(i + 10)) << 8);
        RPDA = 0;
        RPDC = 0;
        RPER1 = 0;
        RPER3 = 0;
        if (i < 4)
            RPDS = 0x11C0;

        // init the drive info based on the declaired type
        switch (attached_drives[i])
        {
        case RP02:  //??MB
            sectors[i] = 10;
            surfaces[i] = 20;
            cylinders[i] = 204;
            wordspsector[i] = NUM_WRD_P_SEC;
            break;
        case RP03:  //??MB
            sectors[i] = 10;
            surfaces[i] = 20;
            cylinders[i] = 406;
            wordspsector[i] = NUM_WRD_P_SEC;
            break;

        case RS03:  // 0.5MB
            sectors[i] = 64;
            surfaces[i] = 1;
            cylinders[i] = 64;
            wordspsector[i] = 64;
            break;
        case RS04:  // 1MB
            sectors[i] = 64;
            surfaces[i] = 1;
            cylinders[i] = 64;
            wordspsector[i] = 128;
            break;

        case RP04:  // 88MB
            sectors[i] = 22;
            surfaces[i] = 19;
            cylinders[i] = 411;
            wordspsector[i] = NUM_WRD_P_SEC;
            break;
        case RP05:  // 88MB
            sectors[i] = 22;
            surfaces[i] = 19;
            cylinders[i] = 411;
            wordspsector[i] = NUM_WRD_P_SEC;
            break;
        case RP06:  // 176MB
            sectors[i] = 22;
            surfaces[i] = 19;
            cylinders[i] = 815;
            wordspsector[i] = NUM_WRD_P_SEC;
            break;

        case RM02:
        case RM03:
            sectors[i] = 32;
            surfaces[i] = 5;
            cylinders[i] = 823;
            wordspsector[i] = NUM_WRD_P_SEC;
            break;

        case RM05:
            sectors[i] = 32;
            surfaces[i] = 19;
            cylinders[i] = 823;
            wordspsector[i] = NUM_WRD_P_SEC;
            break;

        case RM80:
            sectors[i] = 31;
            surfaces[i] = 14;
            cylinders[i] = 559;
            wordspsector[i] = NUM_WRD_P_SEC;
            break;

        case RP07:  // 516MB
            sectors[i] = 50;
            surfaces[i] = 32;
            cylinders[i] = 630;
            wordspsector[i] = NUM_WRD_P_SEC;
            break;

        default:
        }
    }
}

int read16(uint32_t a)
{
    int idx = RPCS2 & 07;  // idx = drive

    switch (a)
    {
    case DEV_RH_CS1:
        result = (RPCS1 & ~0xb01) | ((RPBA >>> 8) & 0x300);
        if (RPDS[idx] & 0x100)
        {
            result |= 0x800;  // DVA depends on drive number
            if (!(RPCS1 & 0x80))
                result |= 1;  // go is opposite of rdy
        }
        else
        {
            result &= 0xff7f;  // rdy off if no dva
        }
        RPCS1 = result;
        if (data >= 0)
        {
            result = insertData(result, physicalAddress, data, byteFlag);
            if (result >= 0)
            {
                RPBA = (RPBA & 0x3cffff) | ((result << 8) & 0x30000);
                result = (result & ~0xb880) | (RPCS1 & 0xb880);
                if (!(result & 0x40))
                    interrupt(-1, 0, 0o254, 0);  //remove pending interrupt if IE not set
                if ((data & 0xc0) == = 0xc0)
                    interrupt(8, 5 << 5, 0o254, 0);  // RB:
                RPCS1 = result;
                if (result & 1 && (RPCS1 & 0x80))
                {
                    rp11_go();
                }
            }
        }
        return;
    case DEV_RH_WC:  // RPWC  Word count
        RPWC = v;
        return;
    case DEV_RH_BA:  // RPBA  Memory address
        result = RPBA & 0xffff;
        if (data >= 0)
        {
            result = insertData(result, physicalAddress, data, byteFlag);
            if (result >= 0)
            {
                RPBA = (RPBA & 0x3f0000) | (result & 0xfffe);  // must be even
            }
        }
        return;
    case DEV_RH_CS2:  // RPCS2 Control status 2
        result = RPCS2;
        if (data >= 0)
        {
            result = insertData(result, physicalAddress, data, byteFlag);
            if (result >= 0)
            {
                RPCS2 = (result & 0x3f) | (RPCS2 & 0xffc0);
                if (result & 0x20)
                    rp11_init();
            }
        }
        break;
    case DEV_RH_AS:  // RPAS  Attention summary
        result = 0;
        for (idx = 0; idx < NUM_RP_DRIVES; idx++)
        {
            if (RPDS[idx] & 0x8000)
            {
                if (data >= 0 && (data & (1 << idx)))
                {
                    RPDS[idx] &= 0x7fff;
                }
                else
                {
                    result |= 1 << idx;
                }
            }
        }
        if (data > 0)
            RPCS1 &= 0x7fff;  // Turn off SC
        break;
    case DEV_RH_DB:  // RPDB  Data buffer
        result = 0;
        break;
    case DEV_RH_BAE:  // RPBAE Bus address extension
        result = (RPBA >>> 16) & 0x3f;
        if (data >= 0)
        {
            result = insertData(result, physicalAddress, data, byteFlag);
            if (result >= 0)
            {
                RPBA = ((result & 0x3f) << 16) | (RPBA & 0xffff);
            }
        }
        break;
    case DEV_RH_CS3:  // RPCS3 Control status 3
        // result = insertData(RPCS3, physicalAddress, data, byteFlag);
        // if (result >= 0) RPCS3 = result;
        result = 0;
        break;
    default:
        if (RPDS[idx] & 0x100)
        {
            switch (a)
            {                // Drive registers which may or may not be present
            case DEV_RH_DA:  // RPDA  Disk address
                result = insertData(RPDA[idx], physicalAddress, data, byteFlag);
                if (result >= 0)
                    RPDA[idx] = result & 0x1f1f;
                break;
            case DEV_RH_DS:  // RPDS  drive status
                result = RPDS[idx];
                break;
            case DEV_RH_ER1:  // RPER1 Error 1
                result = 0;   // RPER1[idx];
                break;
            case DEV_RH_LA:  // RPLA  Look ahead
                result = 0;  // RPLA[idx];
                break;
            case DEV_RH_MR:  // RPMR  Maintenance
                //result = insertData(RPMR[idx], physicalAddress, data, byteFlag);
                //if (result >= 0) RPMR[idx] = result & 0x3ff;
                result = 0;
                break;
            case DEV_RH_DT:                     // RPDT  drive type read only
                result = attached_drives[idx];  // 0o20022
                break;
            case DEV_RH_SN:  // RPSN  Serial number read only - lie and return drive + 1
                result = (idx + 1) + (1 << 4) + (((uint8_t)(idx + 10)) << 8);
                return;
            case DEV_RH_OF:  // RPOF  Offset register
                RPOF[idx] = v;
                return;
            case DEV_RH_DC:  // RPDC  Desired cylinder
                result = insertData(RPDC[idx], physicalAddress, data, byteFlag);
                if (result >= 0)
                    RPDC[idx] = result & 0x3ff;
                break;
            case DEV_RH_CC:  // RPCC  Current cylinder read only - lie and used desired cylinder
                result = RPDC[idx];
                break;
            case DEV_RH_ER2:  // RPER2 Error 2
                result = 0;
                break;
            case DEV_RH_ER3:  // RPER3 Error 3
                result = 0;   // RPER3[idx];
                break;
            case DEV_RH_EC1:  // RPEC1 Error correction 1 read only
                result = 0;   // RPEC1[idx];
                break;
            case DEV_RH_EC2:  // RPEC2 Error correction 2 read only
                result = 0;   //RPEC2[idx];
                break;
            default:
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% rp11 write16: invalid write address"));
                }
                // panic();
                longjmp(trapbuf, INTBUS);
            }
        }
        else
        {
            RPCS2 |= 0x1000;  // NED
            RPCS1 |= 0xc000;  // SC + TRE
            if (RPCS1 & 0x40)
            {
                procNS::interrupt(INTRP, 5);
            }
        }
    }
}

void write16(uint32_t a, uint16_t v)
{
    int idx = RPCS2 & 07;  // idx = drive

    switch (a)
    {
    case DEV_RH_CS1:
        RPBA = (RPBA & 0x3cffff) | ((v << 8) & 0x30000);
        v = (v & ~0xb880) | (RPCS1 & 0xb880);
        if (!(v & 0x40))
            interrupt(-1, 0, 0o254, 0);  //remove pending interrupt if IE not set
        if ((v & 0xc0) == 0xc0)
            interrupt(8, 5 << 5, 0o254, 0);  // RB:
        RPCS1 = v;
        if (v & 1 && (RPCS1 & 0x80))
        {
            step();
        }
        return;
    case DEV_RH_WC:  // RPWC  Word count
        RPWC = v;
        return;
    case DEV_RH_BA:  // RPBA  Memory address
        RPBA = (RPBA & 0x30000) | (v);
        return;
    case DEV_RH_CS2:  // RPCS2 Control status 2
        RPCS2 = (v & 0x3f) | (RPCS2 & 0xffc0);
        if (v & 0x20)
            reset();
        return;
    case DEV_RH_AS:  // RPAS  Attention summary
        for (idx = 0; idx < NUM_RP_DRIVES; idx++)
        {
            if (RPDS[idx] & 0x8000)
            {
                if (v >= 0 && (v & (1 << idx)))
                {
                    RPDS[idx] &= 0x7fff;
                }
            }
        }
        if (v > 0)
            RPCS1 &= 0x7fff;  // Turn off SC
        return;
    case DEV_RH_DB:  // RPDB  Data buffer
        RPDB = v;
        return;
    case DEV_RH_BAE:  // RPBAE Bus address extension
        RPBA = ((v & 0x3f) << 16) | (RPBA & 0xffff);
        return;
    case DEV_RH_CS3:  // RPCS3 Control status 3
        RPCS3 = v;
        return;
    default:
        if (RPDS[idx] & 0x100)
        {
            switch (a)
            {                // Drive registers which may or may not be present
            case DEV_RH_DA:  // RPDA  Disk address
                result = insertData(RPDA[idx], physicalAddress, data, byteFlag);
                if (result >= 0)
                    RPDA[idx] = result & 0x1f1f;
                break;
            case DEV_RH_DS:  // RPDS  drive status
                result = RPDS[idx];
                break;
            case DEV_RH_ER1:  // RPER1 Error 1
                result = 0;   // RPER1[idx];
                break;
            case DEV_RH_LA:  // RPLA  Look ahead
                result = 0;  // RPLA[idx];
                break;
            case DEV_RH_MR:  // RPMR  Maintenance
                //result = insertData(RPMR[idx], physicalAddress, data, byteFlag);
                //if (result >= 0) RPMR[idx] = result & 0x3ff;
                result = 0;
                break;
            case DEV_RH_DT:                     // RPDT  drive type read only
                result = attached_drives[idx];  // 0o20022
                break;
            case DEV_RH_SN:  // RPSN  Serial number read only - lie and return drive + 1
                result = (idx + 1) + (1 << 4) + (((uint8_t)(idx + 10)) << 8);
                break;
            case DEV_RH_OF:  // RPOF  Offset register
                result = insertData(RPOF[idx], physicalAddress, data, byteFlag);
                if (result >= 0)
                    RPOF[idx] = result;
                //result = 0x1000;
                break;
            case DEV_RH_DC:  // RPDC  Desired cylinder
                result = insertData(RPDC[idx], physicalAddress, data, byteFlag);
                if (result >= 0)
                    RPDC[idx] = result & 0x3ff;
                break;
            case DEV_RH_CC:  // RPCC  Current cylinder read only - lie and used desired cylinder
                result = RPDC[idx];
                break;
            case DEV_RH_ER2:  // RPER2 Error 2
                result = 0;
                break;
            case DEV_RH_ER3:  // RPER3 Error 3
                result = 0;   // RPER3[idx];
                break;
            case DEV_RH_EC1:  // RPEC1 Error correction 1 read only
                result = 0;   // RPEC1[idx];
                break;
            case DEV_RH_EC2:  // RPEC2 Error correction 2 read only
                result = 0;   //RPEC2[idx];
                break;
            default:
                if (PRINTSIMLINES)
                {
                    Serial.println(F("%% rp11 write16: invalid write address"));
                }
                // panic();
                longjmp(trapbuf, INTBUS);
            }
        }
        else
        {
            RPCS2 |= 0x1000;  // NED
            RPCS1 |= 0xc000;  // SC + TRE
            if (RPCS1 & 0x40)
            {
                procNS::interrupt(INTRP, 5);
            }
        }
    }
}

int step()
{
again:
    int count, val;
    int8_t w = -1;

    drive = RPCS2 & 07;
    cylinder = RPDC[drive];
    surface = (RPDA[drive] >> 8);
    sector = (RPDA[drive] & 0377);

    // the drive isn't attached! We can't use this!
    if (!attached_drives[drive])
    {
        RPCS1 |= 0140000;
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% rp11 step: tried to read invalid disk"));
        }
        return;
    }

    RPDS[drive] &= 077777;

    switch (RPCS1 & 077)
    {
    case 00:  // NOP
    case 01:  // NULL
        return;
    case 03:  // Unload
    case 05:  // Seek
    case 07:  // Recalibrate
        break;
    case 011:  // Initialise
        RPDS[drive] = 010700;
        RPCS1 &= ~070077;  // clear errors
        RPDA[drive] = 0;
        RPDC[drive] = 0;
        RPCS1 = 04200;  // ?WHY?
        return;
    case 013:  // release
        return;
    case 015:  // offset
    case 017:  // return to centre
        break;
    case 021:  // Read in preset
        RPDC[drive] = 0;
        RPDA[drive] = 0;
        RPDS[drive] = 010700;
        RPOF[drive] = 0;
        return;
    case 023:  // pack
        RPDS[drive] |= 0100;
        return;
    case 031:  //search
        break;
    case 061:  // write
        w = 1;
        break;
    case 071:  // read
        w = 0;
        break;
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% unimplemented RP11/RH11 operation"));
        }
        //panic();
        procNS::interrupt(INTRP, 5);
        return;
    }

    if (DEBUG_RP11)
    {
        if (PRINTSIMLINES)
        {
            Serial.print("%% rp11 step: RPBA: ");
            Serial.print(RPBA, DEC);
            Serial.print(" RPWC: ");
            Serial.print(RPWC, DEC);
            Serial.print(" cylinder: ");
            Serial.print(cylinder, DEC);
            Serial.print(" sector: ");
            Serial.print(sector, DEC);
            Serial.print(" write: ");
            Serial.println(w ? "true" : "false");
        }
    }

    if (w != -1)  // write or read
    {
        // if address is beyond disk
        if (rpsize(drive) < rppos(drive))
        {
            RPER1[drive] |= 02000;  // invalid address
            RPCS1 |= 0140000;
            if (PRINTSIMLINES)
            {
                Serial.println(F("%% rp11 step: tried to read beyond disk"));
            }
            return;
        }

        RPCS1 &= ~070000;
        RPCS1 &= ~040200;
        RPCS2 &= ~04000;
        RPDS[drive] &= ~02200;

        int pos = rppos(drive);

        if (!rpdata[drive].seekSet(pos))
        {
            if (PRINTSIMLINES)
            {
                Serial.println(F("%% rp11 step: failed to seek in file"));
            }
            panic();
        }

        for (i = 0; i < (NUM_WRD_P_SEC / 2) && RPWC != 0; i++)
        {
            // write
            if (w == 1)
            {
                val = dd11::read16(RPBA);
                rpdata[drive].write(val & 0xFF);
                rpdata[drive].write((val >> 8) & 0xFF);
            }
            // read
            else if (w == 2)
            {
                int t = rpdata[drive].read();
                if (t == -1)
                {
                    if (PRINTSIMLINES)
                    {
                        Serial.println(F("%% rp11 step: failed to read file (low)"));
                    }
                    panic();
                }
                val = t & 0xFF;

                t = rpdata[drive].read();
                if (t == -1)
                {
                    if (PRINTSIMLINES)
                    {
                        Serial.println(F("%% rp11 step: failed to read file (high)"));
                    }
                    panic();
                }
                val += ((t & 0xFF) << 8);

                dd11::write16(RPBA, val);
            }
            RPBA += 2;
            RPWC = (RPWC + 1) & 0xFFFF;
        }

        sector++;
        if (sector > sectors[drive] - 1)
        {
            sector = 0;
            surface++;
            if (surface > surfaces[drive] - 1)
            {
                surface = 0;
                cylinder++;
                if (cylinder > cylinders[drive] - 1)
                {
                    RPER1[drive] |= 02000;  // invalid address
                    RPCS1 |= 0140000;
                    if (PRINTSIMLINES)
                    {
                        Serial.println(F("%% rp11 step: tried to io beyond disk"));
                    }
                }
            }
        }

        if (RPWC == 0)
        {
            RPCS2 &= ~07;
            RPCS2 |= drive & 07;
            RPDC[drive] = cylinder;
            RPDA[drive] = (surface & 0377 << 8);
            RPDA[drive] |= sector & 0377;

            rpready();
            if (RPCS1 & 0x40)
            {
                procNS::interrupt(INTRP, 5);
            }
        }
        else
        {
            goto again;
        }
    }
    return;
}
};  // namespace rh11
#endif
