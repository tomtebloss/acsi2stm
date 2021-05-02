How to use ACSI2STM
===================

This quick start guide will explain how to use an aready working ACSI2STM unit.

There are 2 ways to use ACSI2STM: using floppy images or as a hard drive. Floppy images are easier to work with but have
limitations. See [FloppyEmulator](FloppyEmulator.md) if you are interested.

Using a hard drive on the Atari requires a hard drive driver. The following drivers are known to work:

 * The original AHDI driver. This has a lot of limitations, you should avoid it.
 * The ICD PRO drivers. It formats hard drives to the Atari format.
 * The [Peter Putnik ACSI driver](https://atari.8bitchip.info/astams.php). It allows using standard FAT partitions.


Creating a bootable hard drive with Peter Putnik's driver
---------------------------------------------------------

 * Download the ACSID07.ZIP file from [Peter Putnik drivers page](https://atari.8bitchip.info/astams.php).
 * Format a SD card with a single FAT16 partition.
 * Unzip to the root of that SD card.
 * Create a folder named AUTO and move BIGDOS.PRG in it.
 * If you prefer using a real floppy disk:
   * Copy AUBACD07.PRG from the zip file to the floppy.
 * If you prefer using a floppy image:
   * Create an Atari floppy image (st format) and copy AUBACD07.PRG on it.
   * Create a folder named ACSI2STM to the root of the SD card and copy the floppy image in it.
   * Rename the image to "DRIVE_A.ST".
 * Boot the Atari with the SD card inserted (and the floppy disk if you chose a real floppy disk).
 * On the GEM desktop, open drive A.
 * Run AUBACD07.PRG.
 * Press "I" to install the driver.


Creating a bootable hard drive with ICD PRO drivers
---------------------------------------------------

Download a floppy disk of ICD PRO drivers. They are available online as ST floppy images. Transfer it to a real floppy your ST can
read.

*WARNING*: The floppy emulator will work, but some partition operations will fail once the drive is made bootable.
See [FloppyEmulator](FloppyEmulator.md) for details.

 * Run ICDFMT.PRG from the floppy.
   * Select the hard drive you wish to format in the list.
   * In the next screen, click "Verify passes" and set it to 0.
   * Click "Format", then "Continue".
   * The tool will create default partitions. The default settings are terrible: click "Clear" under "Size options".
   * Create your partitions by entering their size in the "Size" column. See below for partition size limitations.
   * Once you are happy, click "Partition entire disk".
   * Don't print the partition table: click "Cancel".
   * Exit to GEM by clicking "Quit".
 * Run HDUTIL.PRG.
   * Click "Boot" to make the disk bootable, select the "C" partition.
   * When prompted for ICDBOOT.PRG, accept the defaults and click OK.
 * (optional) in HDUTIL.PRG, click "Configure".
   * Click C:\ICDBOOT.SYS.
   * You can disable caching because ACSI2STM is so fast...
   * You can also disable unused device ids in "Skip ID(s)", it will improve boot time.
   * Click "Save" when you are finished.

Maximum partition sizes are the following:

 * 32MB (65532 sectors) for TOS 1.04 (ST and STF series)
 * 64MB for modern Linux kernels
 * 512MB (1048527 sectors) for TOS 1.62 and 2.06 (STE series)

Other TOS versions were not tested.

With different drivers, you may have different limits. This bridge supports 32 bits access for disks bigger than 8GB.


Working with image files
------------------------

Instead of using a raw SD card, you can use an image file instead.

Place a file named acsi2stm.img at the root of a standard SD card. The file must not be empty and must be a multiple of 512 bytes
to be detected as an image.

If the image file is in use, the ACSI unit name will contain IMG. In that case, the reported size is the size of the image, not
the size of the whole SD card.

The image is exposed as a raw device with no header. This is the same format as used in the Hatari emulator, making the image
directly compatible. You can also transfer data between a raw SD card and an image using tools like Win32 Disk Imager (for Windows)
or the dd command under Linux.

File system size limitations only apply on the Atari file system inside the image. The SD card itself can be of any size and it can
contain any amount of data other than the image itself.

The only downside to use images is performance. There will be a performance impact when using images because of the extra file
system layer.

Hard drive images can be combined with floppy images, even on the same SD card. See [FloppyEmulator](FloppyEmulator.md).
