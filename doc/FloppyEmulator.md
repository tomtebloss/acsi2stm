ACSI2STM floppy emulator
========================

ACSI2STM supports mounting floppy images by hooking block device driver calls. While this is not bullet proof (it will fail for
programs that do direct hardware access) it's a nice convenience to mount *.st files easily.

This driver supports SD card hotplug and falls back to the hardware drive when no SD card is inserted or no image is found.

Floppy images can be up to 32M and must be formatted using FAT12.


How to use
----------

Format your SD card in FAT32 or EXFAT, create a folder named "acsi2stm" and put a floppy image file named "drive_a.st" in it.

You can also put an image named "drive_b.st" to emulate drive B. The driver is smart enough to enable or disable the popup that
asks to insert floppy B in drive A as required.


How not to use
--------------

If you have a hard drive driver that mounts the SD card containing floppy images don't touch the floppy image file while the floppy
drive is being accessed.

Low level tools such as disk formatters/copiers are unlikely to work. Some can be fixed by improving the driver, others cannot be
supported at all because they do direct hardware access.


Supported image formats
-----------------------

The driver only supports raw floppy images (a.k.a. *.st files). The file must be a multiple of 512 bytes or it won't load at all.

The driver will try to guess floppy geometry based on the file size when accessing the image using low level track/side/sector
addressing scheme.


What if I have more than 1 SD card with floppy images ?
-------------------------------------------------------

The driver will try to load images from devices in the reverse order: it tries the last SD card slot first, then goes back to the
first slot until it finds a suitable floppy image.

You can put drive A and drive B images on separate SD card readers.

