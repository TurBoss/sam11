# PDP-11/40 Simulator: sam11

The sam11 software is a cross-platform MCU implementation for software \
emulation of a PDP-11/40\* and some of the supporting hardware.

\* = With some PDP-11/45 and other model stuff thrown in...

At the moment it is _heavily_ based on the avr11 simulator, with changes to \
move the device and file structure to match those of an actual PDP 11 computer.

The original avr11 software supported UNIXv5 and UNIXv6, and this should to.

However the extended hardware modules should allow implementing the missing \
hardware modules easier to allow use of newer operating systems that rely on \
currently unimplemented hardware features.

It derives from 's avr11 software, and is inspired by various emulators.

The structure, however, was re-written based on actual PDP physical structure,
device names, and data paths from the PDP-11/40 manual in order to be more \
useful for learning the system.

The software is designed to be programmed to a device using the arduino IDE \
.ino file, or other compatible editors/IDEs.

See pdp1140.h for more information about file names/splits and pdp-11/40 \
device structure.

Recommended reading:

- https://pdos.csail.mit.edu/6.828/2005/readings/pdp11-40.pdf

- http://gunkies.org/wiki/

- https://gunkies.org/wiki/PDP-11/40

- https://www.pdp-11.nl/

- https://www.pdp-11.nl/pdp11-35startpage.html

- https://en.wikipedia.org/wiki/PDP-11

- https://en.wikipedia.org/wiki/PDP-11_architecture

- http://www.classiccmp.org/pipermail/cctech/2016-November/023935.html
