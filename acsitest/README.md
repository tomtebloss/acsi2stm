ACSI2STM DMA transfer stress test
=================================

This program blasts DMA reads and writes non-stop with various data patterns. It checks that read data is identical to written
data.

In order to work, it uses the internal RAM buffer of the STM32, so it does *NOT* modify any data on SD cards.

To assemble, use vasm on PC or Devpac on Atari.
