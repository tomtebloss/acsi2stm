rm -f boot.bin boot.h driver.bin

# Compile the driver
vasmm68k_mot -devpac -showopt -Fbin -L driver.lst -o driver.bin emudrive/driver.s

[ -e driver.bin ] || exit $?
if grep 'There have been no errors.' driver.lst &>/dev/null; then

  # Compile the boot sector
  vasmm68k_mot -devpac -showopt -Fbin -L boot.lst -o boot.bin emudrive/boot.s

  [ -e boot.bin ] || exit $?
  if grep 'There have been no errors.' boot.lst &>/dev/null; then

    # Generate the source code from the binary blob
    xxd -i boot.bin > acsi2stm/boot.h

    echo "Compilation successful"
  fi
fi
