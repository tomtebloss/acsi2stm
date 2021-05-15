Compiling and flashing the software for ACSI2STM
================================================

For a tutorial on how to build the hardware part, see the [Hardware](Hardware.md) page.

Hardware needed
---------------

 * A STM32 board.
 * A USB-serial dongle for programming the STM32 chip, with a 3.3V USART.
 * Do *NOT* connect USB data lines on the STM32. Use the +5V or +3.3V pin to power it if you are unsure.

You may use STLink, but I had no success using it, some other people did, your mileage may vary.


Software needed
---------------

 * The Arduino software with the Roger Clark Melbourne STM32 library.
 * The SdFat Arduino library by Bill Greiman.
 * A hard drive driver for your Atari ST:
   * AHDI should work but is not recommended.
   * ICD pro driver is tested.
   * Peter Putnik's driver is reported to work.


Installing software
-------------------

Install the [STM32 library](https://github.com/rogerclarkmelbourne/Arduino_STM32/wiki/Installation). The doc says that it only
works on Arduino 1.8.5 but that does seems to work with more recent versions too (confirmed working with Arduino 1.18.15).

In the Tools / Manage Libraries menu of the Arduino interface, search for "SdFat" and install "SdFat by Bill Greiman".

In the Tools menu of the Arduino interface, select the following:

 * Board: Generic STM32F103C series
 * Variant: STM32F103C8
 * Upload method: Serial
 * CPU speed: 72MHz (normal)
 * Optimize: Faster (-O2)
 * Port: your USB serial dongle

Note: you can use any setting in the "Optimize" menu. O2 is recommended for fastest performance, O3 does not bring any speed
improvement but generates much bigger code.

If you have different options in the Tools menu, it may be because you don't have the correct board installed.

Then, you will be able to upload the program to the STM32.


Programming the STM32
---------------------

Set the USB dongle to 3.3V if you have a jumper for that. Connect TX to PA10, RX to PA9 and the GND pins together.

On the STM32 itself, set the BOOT0 jumper to 1 to enable the serial flash bootloader. Reset the STM32 then click Upload.

Once the chip is programmed, switch the BOOT0 jumper back to 0.

Note: the debug output sends data at 115200bps. Set the serial monitor accordingly.


Compile-time options
--------------------

The file acsi2stm/acsi2stm.h contains a few #define that you can change. They are described in the source itself.

Settings that you might wish to change:

 * ACSI_DEBUG: Enables debug output on the serial port. Moderate performance penalty.
 * ACSI_VERBOSE: Requires ACSI_DEBUG. Logs all commands and low-level trafic on the serial port. High performance penalty.
 * IMAGE_FILE_NAME: The image file to use as a hard disk image on the SD card.
 * DRIVE_A_FILE_NAME: The image file to use as a floppy disk image. Same for drive B.
 * ACSI_BOOT_OVERLAY: Turn on or off the boot sector overlay to load the integrated driver.

If you need a reference debug output, see the debug_output.txt file. This contains a full trace of a standard ICD PRO setup booting.


Rebuilding the emudrive blob
----------------------------

To build the driver, you need [vasm](http://sun.hasenbraten.de/vasm/).

You need the _m68k_ CPU with the _mot_ syntax. Build it using the following command:

    make CPU=m68k SYNTAX=mot

Once compiled, put vasmm68k_mot anywhere in your PATH.


Generating the release zip package
----------------------------------

The build_relase.sh script is provided. It's a bash script for Linux, but if you use any other platform, you
can run the vasmm68k_mot and xxd commands by hand. If you have a script for other platforms, contributions are welcome.

Note: this script will modify acsi2stm.h to patch ACSI2STM_VERSION from the VERSION file.  It will also regenerate acsi2stm/boot.h with the new driver payload.
