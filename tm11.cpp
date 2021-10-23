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

// sam11 software emulation of DEC PDP-11/40 TM11 TM Tape Controller

#if USE_TM

#include "tm11.h"

#define TM_DELAY 5000 /* delay in microseconds = 1 ms */

#define NUM_TM_DRIVES (4)

namespace tm11 {

enum
{
    TM_CRDY = 0200,
    TM_IE = 0100,
    TM_CE = 0100000,
};

enum
{
    TMTUR = 01,
    TMWRL = 04,
    TMBOT = 040,
    TMSELR = 0100,
    TMNXM = 0200,
    TMEOT = 02000,
    TMEOF = 040000,
    TMILC = 0100000,
};

uint16_t TMER;
uint16_t TMCS;
uint16_t TMBC;
uint16_t TMBA;
uint16_t TMDB;
uint16_t TMRD;

SdFile tmdata[NUM_TM_DRIVES];
bool attached_drives[NUM_TM_DRIVES];

int cur_tape_no = -1;

void tmnotready()
{
    if (TMCS & 0x40)
    {
        procNS::interrupt(INTTM, 5);
    }
    else
    {
        // no interrupt, just mark as done
        tmready();
    }
}

uint16_t tmready()
{
    TMER |= 1;
    TMCS |= 0x80;
    return TMCS & 0x40;
}

int reset()
{
    TMCS = 0x6080;
    TMER = 0x65;
    cur_tape_no = 0;
    for (int i = 0; i < NUM_TM_DRIVES; i++)
    {
        if (attached_drives[i])
            tmdata[i].seek(0);
    }
}

_next_tape(uint8_t fwd)
{
    //fwd: -1 = reset to 0; 0 = reverse; 1 = forward

    char fullpath[BUFSIZ];

    strcpy(fullpath, tm.path);
    sprintf(&fullpath[strlen(fullpath)], ".%d", cur_tape_no);

    if (tm.fp != NULL)
        fclose(tm.fp);
    if ((tm.fp = fopen(fullpath, "a+")) == NULL)
    {
        if ((tm.fp = fopen(fullpath, "r")) == NULL)
        {
            TMER |= TM_EOT;
            cur_tape_no = -1;
        }
        else
        {
            TMER |= TM_WRL;
        }
    }
    else
    {
        rewind(tm.fp);
        TMER &= ~TM_WRL;
    }
    TMER |= TM_SELR;
    if (cur_tape_no == 0)
        TMER |= TM_BOT;
}

int read16(uint32_t a)
{
    switch (a)
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

finish(uint16_t info)
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

/*
 * tm_do_sforw() - We'll sleeze through this one too, assume record size
 * is always 512 bytes for the space forward command.
 */

void _forward()
{
    unsigned count;

    TMER &= ~TM_EOF;
    count = (0177777 - TMBC) + 1;
    while (count-- && attached_drives[cur_tape_no])
    {
        TMER &= ~TM_BOT;
        if (fread(tm_buffer, 512, 1, tm.fp) == 0)
        {
            TMER |= TM_EOF;
            cur_tape_no++;
            _next_tape(1);
            break;
        }
    }
    TMBC = (0177777 - count) + 1;
}

/*
 * tm_do_srev() - Required by the 2.9 tm driver during writes.  The
 * routine tm_close writes two EOFs then backspaces over the last one.
 * We'll try to simulate the backspace just for this special case.
 */

void _reverse()
{
    uint count;

    TMER &= ~TM_EOF;
    count = (0177777 - TMBC) + 1;
    --count;
    TMER &= ~TM_BOT;
    if (cur_tape_no > 0)
    {
        cur_tape_no--;
        _next_tape(0);
    }
    TMBC = (0177777 - count) + 1;
}

/*
 * tm_do_read() - Perform a read.  We can simplify things
 * by assuming that the number of bytes asked for will be
 * equal to what the the record size was on the real tape.
 * This is a _very_ big assumption, read the tape at your own
 * risk.
 */

void _read()
{
    unsigned count;
    unsigned bytes;
    c_addr addr;
    unsigned t;

    TMER &= ~TM_EOF;
    count = (0177777 - TMBC) + 1;
    addr = TMBA;
    addr |= ((c_addr)(TMCS & 060)) << 12;

    while (count && (tm.fp != NULL))
    {
        TMER &= ~TM_BOT;
        bytes = (count >= 512) ? 512 : count;
        count -= bytes;
#ifndef SWAB
        if (fread(tm_buffer, bytes, 1, tm.fp) == 0)
        {
#else
        if (fread(tm_swab, bytes, 1, tm.fp) == 0)
        {
#endif
            TMER |= TM_EOF;
            cur_tape_no++;
            _next_tape(1);
            break;
        }
        else
        {
#ifdef SWAB
            swab((char*)tm_swab, (char*)tm_buffer,
              sizeof(tm_swab));
#endif
            for (t = 0; t < (bytes / 2); ++t)
            {
                if (sc_word(addr, tm_buffer[t]) != OK)
                {
                    TMER |= TM_NXM;
                    return;
                }
                addr += 2;
            }
        }
    }
    TMBC = (0177777 - count) + 1;
}

void _write()
{
    uint count;
    uint bytes;
    uint32_t addr;
    uint t;

    if (TMER & TM_WRL)
    {
        TMER |= TM_ILC;
        return;
    }

    TMER &= ~TM_EOF;
    count = (0177777 - TMBC) + 1;
    addr = TMBA;
    addr |= ((TMCS & 060) << 12);

    while (count && attached_drives[cur_tape_no])
    {
        TMER &= ~TM_BOT;
        bytes = (count >= 512) ? 512 : count;
        count -= bytes;
        for (t = 0; t < (bytes / 2); ++t)
        {
            uint16_t v = tm_buffer[t + 1] + (tm_buffer[t] << 8);
            dd11::write16(addr, v);
            addr += 2;
        }
#ifdef SWAB
        swab((char*)tm_buffer, (char*)tm_swab,
          sizeof(tm_swab));
        if (fwrite(tm_swab, bytes, 1, tm.fp) == 0)
        {
#else
        if (fwrite(tm_buffer, bytes, 1, tm.fp) == 0)
        {
#endif
            TMER |= TM_EOF;
            cur_tape_no++;
            _next_tape(1);
            break;
        }
    }
    if (attached_drives[cur_tape_no])
        tmdata[cur_tape_no].flush();
    TMBC = (0177777 - count) + 1;
}

void _eof()
{
    TMER &= ~TM_EOF;
    cur_tape_no++;
    _next_tape(1);
}

};  // namespace tm11
#endif
