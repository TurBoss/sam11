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

#include "ini.h"

#include "dd11.h"
#include "kb11.h"  // 11/45
#include "kd11.h"  // 11/40
#include "kl11.h"
#include "kw11.h"
#include "ky11.h"
#include "lp11.h"
#include "ms11.h"
#include "pdp1140.h"
#include "platform.h"
#include "rk11.h"
#include "sam11.h"
#include "termopts.h"
#include "xmem.h"

#include <Arduino.h>
#include <SdFat.h>
#include <stdint.h>
#include <string.h>

#if USE_11_45 && !STRICT_11_40
#define procNS kb11
#else
#define procNS kd11
#endif

#if BOOT_SCRIPT

namespace ini {

/* We will change this to have a parallel array with function pointers once others are added */
const char* opts[] = {
  "set",      // Set a device type
  "en",       // enable a device or setting (short)
  "enable",   // enable a device or setting
  "att",      // attach a file to a device (short)
  "attach",   // attach a file to a device
  "bo",       // boot from a device (short)
  "boot",     // boot from a device
  "g",        // get from an address (mem, short)
  "get",      // get from an address (mem)
  "r"         // read from an address (bus + mem, short)
  "read",     // read from an address (bus + mem)
  "d",        // deposit to an address (short)
  "dep",      // deposit to an address (short)
  "deposit",  // deposit to an address
  "jmp",      // jump the processor to an address (short)
  "jump",     // jump the processor to an address
  "h",        // halt the processor (short)
  "halt",     // halt the processor
  ";",        // comment (no op)
  "c",        // comment (no op)
  "nop"       // no operation
};

int _att(int argc, char** argv)
{
    /* need at least three args */
    if (argc >= 3)
    {
        /* attach to rp disk, aliased to rh as well */
        if ((argv[1][0] == 'r' && argv[1][1] == 'p') || (argv[1][0] == 'r' && argv[1][1] == 'h'))
        {
            //     /* get disk number */
            //     int dnum = atoi(argv[1]);
            //     if (dnum < NUM_RP_DRIVES)
            //     {
            //         // Load RK05 Disk image as Read/Write
            //         if (!rh11::rpdata[dnum].open(argv[2], O_RDWR))
            //         {
            //             sd.errorHalt("%% Attaching image to RK disk failed.");
            //             rh11::attached_drives[dnum] = false;
            //         }
            //         else
            //             rh11::attached_drives[dnum] = true;
            //     }
        }
        /* attach to an rk disk */
        else if ((argv[1][0] == 'r' && argv[1][1] == 'k'))
        {
            /* get disk number */
            int dnum = atoi(argv[1]);
            if (dnum < NUM_RK_DRIVES)
            {
                // If it's already open, close it
                if (rk11::attached_drives[dnum])
                {
                    rk11::rkdata[dnum].close();
                }

                // Load RK05 Disk image as Read/Write
                if (!rk11::rkdata[dnum].open(argv[2], O_RDWR))
                {
                    sd.errorHalt("%% Attaching image to RK disk failed.");
                    rk11::attached_drives[dnum] = false;
                }
                else
                {
                    _printf("%%%%\tImage %s attached to %s\r\n", argv[2], argv[1]);
                    rk11::attached_drives[dnum] = true;
                }
            }
        }
    }
    return 0;
}

/* Split string by character token
 * sp = token to split by
 * str = string to split
 * argc = number of resulting substrings
 * argv = list of the resulting substrings
 * len = length of the allowed list of resulting substrings
 * returns = -1 for error, 0 for no error.
 */
int strsplit(char sp, char* str, int* argc, char** argv, int len)
{
    int lib, idx;
    /* Used for checking if the line is blank */
    lib = 1;

    /* Clear the number of arguments collected */
    *argc = 0;

    /* Clear the argv array */
    for (idx = 0; idx < len; idx++)
    {
        argv[idx] = (char*)malloc(sizeof(char) * 1);
        argv[idx][0] = 0;
    }

    /* cut off blank spaces from the end of the string (just change to 0) */
    for (idx = strlen(str) - 1; idx >= 0; idx--)
    {
        if (isblank(str[idx]))
            str[idx] = 0;
        else
        {
            /* If it's not a blank character, then the line has data and we
			 * break */
            lib = 0;
            break;
        }
    }

    /* If the line is blank, then there are no arguments */
    if (lib)
    {
        return -1;
    }
    else
    {
        /* Go through the entered line and split it by string into an argv array
		 */
        for (idx = 0; (idx < strlen(str)) && (*argc) < len; idx++)
        {
            /* Replace tabs with token */
            if (str[idx] == '\t')
            {
                str[idx] = sp;
            }

            /* if the previous character was a space, ignore this one and don't
			 * add it to args */
            if (idx > 0 && str[idx] == sp && str[idx - 1] == sp)
            {
            }
            /* If we've hit a separator, move ontothe next entry (ignore if
			 * we're still on the first character)*/
            else if (idx > 0 && str[idx] == sp)
            {
                (*argc)++;
                if ((*argc) >= len)
                    break;
            }
            /* If it's a printable character add it to the current argument */
            else if (isprint(str[idx]))
            {
                strncat(argv[(*argc)], &str[idx], 1);
            }
        }

        /* If the line has non-space characters, we must have at least one arg,
		 * so add it */
        if ((*argc) < len)
        {
            (*argc)++; /* always at least 1 arg */
        }
    }
    return 0;
}

int setup_fr_line(char* line)
{
    int len = 5;                                        // max 5 arguments
    int argc;                                           // number of arguments
    char** argv = (char**)malloc(sizeof(char*) * len);  // array of stored elements

    // if we split the line
    if (!strsplit(' ', line, &argc, argv, len))
    {
        // Attach
        if (!strcmp("att", argv[0]) || strcmp("attach", argv[0]))
        {
            _att(argc, argv);
        }
    }
    return 0;
}

};  // namespace ini

#endif