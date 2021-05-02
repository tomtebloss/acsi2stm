ACSI2STM floppy emulator
========================

ACSI2STM supports mounting floppy images by hooking block device driver calls. While this is not bullet proof, and it will fail for
programs that do direct hardware access, it's a nice convenience to mount *.st files easily.

This driver supports SD card hotplug and falls back to the hardware drive when no SD card is inserted or no image is found.

Floppy images can be up to 32M and must be formatted using FAT12.


How to use
----------

Format your SD card in FAT32 or EXFAT, create a folder named "acsi2stm" and put a floppy image file named "drive_a.st" in it.

You can also put an image named "drive_b.st" to emulate drive B. The driver is smart enough to enable or disable the popup that
asks to insert floppy B in drive A as required.


How not to use
--------------

If you have a hard drive driver that mounts the SD card containing floppy images such as P. Putnik's, it will conflict with the FAT
driver in the STM32. You can use both at the same time, but don't touch the floppy image file if the floppy drive is being
accessed.

Low level tools such as disk formatters/copiers are unlikely to work. Some can be fixed by improving the driver, others cannot be
supported at all because they do direct hardware access.


Supported image formats
-----------------------

The driver only supports raw floppy images (a.k.a. *.st files). The file must be a multiple of 512 bytes or it won't load at all.

The driver will try to guess floppy geometry based on the file size when accessing the image using low level track/side/sector
addressing scheme.


What if I have more than 1 SD card with images ?
------------------------------------------------

The driver will try to load images from devices in the reverse order: it tries the last SD card slot first, then goes back to the
first slot until it finds a suitable floppy image.

You can put drive A and drive B images on separate SD card readers.


How the emulator driver loads and the boot sector overlay issue
---------------------------------------------------------------

### Short version

When the floppy emulator is loaded, you can't modify the boot sector of the hard drive in the first SD card slot if it's bootable.

### Long version

When the Atari ST boots, it checks all ACSI devices in sequence (from id 0 to id 7) for a valid boot sector and runs the code
found in it. Then, the boot code decides whether the ST should continue that sequence and boot the next drive.

The floppy emulator works by injecting its boot code in the boot sector of the first hard drive. If there is no hard drive (empty
SD card), it still returns its boot code, presenting itself as a tiny 512 bytes read-only hard drive. Once this boot code is loaded,
it accesses the STM32 via other ways (different ACSI LUNs) so it does not interact in any way with the hard drive it is attached to.

Once the floppy emulator is loaded, it checks whether the floppy image (if any) or the hard drive is bootable, in that order. If
a valid boot sector is found, it loads the boot sector and runs it. That way, you can have both emulated floppies and a hard drive
driver loaded at the same time.

### Why does this sometimes disables writing the hard drive boot sector

Because of that loading mechanism, reading the hard drive boot sector normally won't return the actual boot sector of the hard
drive, but altered data.

When changing the content of the boot sector (e.g. editing the partition table), most utilities will read the boot sector, make the
changes, and write the whole sector again. In that case, it would read the floppy emulator instead of the hard drive boot, and
totally destroy the boot code.

Since there is no way to tell whether the boot sector is read by the Atari ST boot procedure or a disk utility, writing to the
boot code is simply ignored if it contains the overlay code, which is often the case when trying to change partitions on a bootable
hard drive.

### How do I work around this ?

If you need to change partitions on a bootable disk, there are many options:

 * Recompile with ACSI_BOOT_OVERLAY to 0. This will completely disable the boot overlay mechanism.
 * Make sure the floppy driver doesn't start by removing all floppy images and inserting SD cards in every available slot.
 * Insert the SD card in the second slot.

