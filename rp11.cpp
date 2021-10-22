#if true

#include "rp11.h"

#define TM_DELAY 5000 /* delay in microseconds = 1 ms */

// Many RP registers are independent for each disk!
#define NUM_RP_DRIVES (4)

// Other than for the RP03, the RP disk interface was actually called RH11, but for consistency we've renamed it...
namespace rp11 {

uint16_t RPCS1, RPWC, RPBA, RPCS2, RPDB, RPCS3;

// butload of registers
uint16_t RPDA[NUM_RP_DRIVES];
uint16_t RPDS[NUM_RP_DRIVES];
uint16_t RPER1[NUM_RP_DRIVES];
uint16_t RPLA[NUM_RP_DRIVES];
uint16_t RPMR[NUM_RP_DRIVES];
uint16_t RPDT[NUM_RP_DRIVES];
uint16_t RPSN[NUM_RP_DRIVES];
uint16_t RPOF[NUM_RP_DRIVES];
uint16_t RPDC[NUM_RP_DRIVES];
uint16_t RPCC[NUM_RP_DRIVES];
uint16_t RPER2[NUM_RP_DRIVES];
uint16_t RPER3[NUM_RP_DRIVES];
uint16_t RPEC1[NUM_RP_DRIVES];
uint16_t RPEC2[NUM_RP_DRIVES];

SdFile rpdata[NUM_TM_DRIVES];
uint16_t attached_drives[NUM_RP_DRIVES];  // doubles as DType register
uint16_t drive;

uint16_t sectors[NUM_RP_DRIVES];
uint16_t surfaces[NUM_RP_DRIVES];
uint16_t cylinders[NUM_RP_DRIVES];

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

    RP02 = -2,  // not actually part of the RH11 controller, so we handle these differently.
    RP03 = -3,

    RP04 = MOH + 020,
    RP05 = MOH + 021,
    RP06 = MOH + 022,
    RP07 = MOH + 042,  // ????????? Which kind of disk is this number for?

    RM02 = MOH + 025,
    RM03 = MOH + 024,
    RM05 = MOH + 027,
    RM80 = MOH + 026,
};

void rpattention(uint16_t flg)
{
    RPAS |= 1 << drive;
    RPDS[drive] |= 0x8000;
    if (flg)
    {
        RPER1[drive] |= flg;
        RPDS[drive] |= 0x4000;
    }
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
        case RP02:
            sectors[i] = 10;
            surfaces[i] = 20;
            cylinders[i] = 204;
            break;
        case RP03:
            sectors[i] = 10;
            surfaces[i] = 20;
            cylinders[i] = 406;
            break;

        case RP04:
            sectors[i] = 22;
            surfaces[i] = 19;
            cylinders[i] = 411;
            break;
        case RP06:
            sectors[i] = 22;
            surfaces[i] = 19;
            cylinders[i] = 815;
            break;

        case RP07:
            sectors[i] = 50;
            surfaces[i] = 32;
            cylinders[i] = 630;
            break;

        case RM02:
        case RM03:
            sectors[i] = 32;
            surfaces[i] = 5;
            cylinders[i] = 823;
            break;

        // unsupported disk types
        case RP05:
        case RM05:
        case RM80:
        default:
        }
    }
}

int read16(uint32_t addr, uint16_t word)
{
    switch (addr)
    {
    case DEV_TM_ER:  // status/error
        return TMER;
    case DEV_TM_CS:  // command
        return TMCS;
    case DEV_TM_BC:  // byte record counter
        return TMBC;
    case DEV_TM_BA:  // current memory address
        return TMBA;
    case DEV_TM_DB:  // data buffer
        return TMDB case DEV_TM_RD: return TMRD;
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% tm11 read16: invalid read"));
        }
        // panic();
        return 0;
    }
}

void write16(uint32_t a, uint16_t v)
{
    switch (addr)
    {
    case DEV_TM_ER:  // status/error
        break;
    case DEV_TM_CS:  // command
        TMCS &= ((1 << 7) | 1 << 15);
        TMCS |= v & ~(1 << 7 | 1 << 15);
        // if go is triggered, do it immediately
        if (TMCS & (1 | 1 << 6))
        {
            TMCS &= ~(1 << 7);
            step();
        }
        break;
    case DEV_TM_BC:  // byte record counter
        TMBC = v;
        break;
    case DEV_TM_BA:  // current address
        TMBA = v;
        break;
    case DEV_TM_DB:  // data buffer
        TMDB = v;
        break;
    case DEV_TM_RD:  // read lines
        TMRD = v;
        break;
    default:
        if (PRINTSIMLINES)
        {
            Serial.println(F("%% tm11 write16: invalid write"));
        }
        // panic();
    }
}

int step()
{
    uint tm_delay = TM_DELAY;

    int sector, address, count;
    int drive = (TMCS >> 8) & 3;

    TMCS &= ~0x81;   // ready bit (7!) and go (0)
    TMER &= 0x04fe;  // turn off tape unit ready

    switch ((TMCS >> 1) & 07)
    {
    case 0:  // offline
        break;
    case 1:  // read
        _read();
        break;
    case 2:  // write
    case 3:
        _write();
        break;
    case 6:  // write end of file
        _eof();
        break;
    case 4:  // forward
        _forward();
        break;
    case 5:  // reverse
        _reverse();
        break;
    case 7:  // rewind
        TMER |= 040;
        tmdata[cur_tape_no].seek(0);
        cur_tape_no = 0;
        //        _next_tape(-1);
        break;
    default:
        if (TMCS & TM_GO)
        {
            TMER |= TM_ILC;
        }
        tm_delay = 0;
        break;
    }

    if (TMCS & (1 << 6))
        ev_register(TM_PRI, tm_finish, tm_delay, (d_word)0);
    else
        finish(0);
}

void finish(uint16_t info)
{
    if (TMER & (TM_ILC | TM_EOT | TM_NXM))
        TMCS |= TM_CE;
    TMCS |= TM_CRDY;

    if (TMCS & (1 << 6))
    {
        TMCS &= ~(1 << 6));
        procNS::interrupt(INTTM, , 5);
    }
}

};  // namespace rp11
#endif
