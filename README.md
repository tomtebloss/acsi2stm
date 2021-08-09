**EXPERIMENTAL BRANCH - WORK IN PROGRESS**

Known issues on the integrated driver:
 * Mounting/overlay logic needs a full revamp.
 * Need to test the TOS loader correctly.

Known issues on the filesystem engine:
 * Fsfirst/Fsnext don't work well when creating/deleting files: Breaks recursive directory operations.
 * Pexec is not supported: you can't execute programs from the filesystem.
 * File descriptors are lost/reset when rescanning drives.
 * Filesystem access is very slow, code needs a lot of optimizations.

Known issues on the floppy emulator:
 * Low level access is not good. Formatting and low level copy doesn't work.
 * SD card hot swapping does not always work well.

Feature wish list:
 * RTC support (only needs a battery on the STM32).
 * Automatic drive desktop icon installation for TOS < 2.xx.
 * Dynamic floppy image mounting by hooking Pexec (and DESKTOP.INF auto patching).


ACSI2STM: Atari ST ACSI hard drive emulator
===========================================

This code provides a hard drive emulator for your Atari ST using an inexpensive STM32 microcontroller and a SD card.

The aim of this project is to be very easy to build, extremely cheap, reliable and safe for your precious vintage machine.

The module supports up to 5 SD card readers, showing them as 5 different ACSI devices plugged in. You can choose the ACSI
ID of each SD card by soldering CS wires on the matching STM32 pin.

It can work in 4 ways:
 * Expose a raw SD card as a hard disk to the Atari.
 * Expose a hard disk image file to the Atari.
 * Expose floppy disk images to the Atari with a built-in driver.
 * Expose a SD filesystem as a GEMDOS drive on the Atari.


Folders
-------

Here is how the project is organized:

 * acsi2stm: The arduino project.
 * doc: All the documentation about the project.
 * stmdrive: Sources for the floppy emulator and GEMDOS driver.
 * LICENSE: The license for the whole project.


Documentation
-------------

For a quick start guide on how to use an already built unit, check [HowToUse](doc/HowToUse.md).

For information about how to make the hardware, check [Hardware](doc/Hardware.md).

For information about compiling the firmware with Arduino and uploading it to the STM32, check [Compiling](doc/Compiling.md)

The doc folder contains more documentation, you may wish to browse it if needed.


Credits
-------

I would like to thanks the people that put invaluable information online that made this project possible in a finite amount of
time. Without them, this project would have not existed.

 * The http://atari.8bitchip.info website and his author, who also contributes on various forums.
 * The Hatari developpers. I used its source code as a reference for ACSI commands.
 * The emuTOS developpers. I used its source code for many tricks and lists.
 * The UltraSatan project for their documentation.
 * Sr Antonio, Edu Arana, Frederick321, Ulises74, Maciej G., Olivier Gossuin and Marcel Prisi for their very detailed feedback.
   that helped me a lot for fine tuning the DRQ/ACK signals and other various aspects of the projects.
 * All the kind people sharing their issues on GitHub. That really helps a lot making this project better.
