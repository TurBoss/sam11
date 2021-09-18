PDP-11/40 Simulator: sam11
==========================

The sam11 software is a cross-platform MCU implementation for software emulation of a PDP-11/40\* and some of the supporting hardware.

\* = With some PDP-11/45 and other model stuff thrown in...

The code "skeleton", processor instruction functions, RK11, and mmu is based on a fork of Dave Cheney's avr11 simulator. The JS Emulator folder contains the source for the JS PDP-11/40 emulator by Aiju that avr11 was based on.

The original avr11 software supported UNIXv6, and this should to. As of 2021-09-17 some OSes boot, but crash out for various reasons. UNIXv6 runs, but some programs (e.g. cc, chess) fail due to the same MMU bugs in avr11 and Aiju's emulator.

The structure was re-written from avr11 based on actual PDP physical structure, device names, and data paths from the PDP-11/40 manual in order to be more useful for learning the system, though there is still work to do, especially around MMU and FIS/FP11 processing.

The extended hardware modules and new structure should allow implementing the missing hardware modules easier to allow use of operating systems and configurations that rely on currently unimplemented hardware features or devices.

See pdp1140.h for more information about file names/splits and pdp-11/40 device structure.

It is also inspired/informed by various other emulators such as simh, and nankervis js emulator, but it does not derrive any code from them.

The software is designed to be programmed to a device using the arduino IDE .ino file, or other compatible editors/IDEs.

Because different boards all have different options for where the PDP ram lives, how much, and what pins for LEDs/CSs there is a file platform.h which is used to define these variables, along with a corrosponding platform.cpp file which actuates some of these physical parts. You will need to add new information for chips not currently implemented.

The RAM type is defined in platform.h, and depending the options different cpp files are inserted into ms11.cpp from the ram_opt folder cpp.h files.

Installation:
-------------

If you wish to use this as tested without defining a new board, you will need an Adafruit Grand Central M4, microSD card, and USB cable.

Put the .dsk images from the OS Images folder onto the root of your SD card, without renaming.
Plug the board into your computer, and using the arduino IDE (or compatible, e.g. vscode) compile and upload the sam11.ino file. This should automatically include all the cpp files and headers necessary.

Then simply open up the terminal/serial, type "0." to select the UnixV6 image, type "unix" at the '@' to boot.

Expect a trap at 0760000 as this is by design and is Unix discovering the maximum RAM available (248KB).

Recommended reading:
--------------------

- PDP-11/40 processor handbook: <https://pdos.csail.mit.edu/6.828/2005/readings/pdp11-40.pdf>

- A great PDP/DEC wiki: <http://gunkies.org/wiki/>

- Specific PDPD-11/40 page <https://gunkies.org/wiki/PDP-11/40>

- A collector's info page: <https://www.pdp-11.nl/>

- Specific page on 11/40s (aka 11/35) <https://www.pdp-11.nl/pdp11-35startpage.html>

- Wiki <https://en.wikipedia.org/wiki/PDP-11>

- Wiki with more info <https://en.wikipedia.org/wiki/PDP-11_architecture>

- A discussion about stack memory on PDP-11/45s <http://www.classiccmp.org/pipermail/cctech/2016-November/023935.html>
