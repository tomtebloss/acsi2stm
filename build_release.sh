#!/bin/sh
# This script is a "Works on my computer" script.
# You may have to study and adapt it to run on your computer.
#
#  Commands needed in your path
#
#    sed
#    vasmm68k_mot
#    xxd
#    arduino
#    zip

if ! [ -e acsi2stm/acsi2stm.ino ]; then
  echo "Please run this script from the root directory of the project"
  exit 1
fi

VERSION=`cat VERSION`

sed -i 's/^\(#define ACSI2STM_VERSION\).*/\1 "'$VERSION'"/' acsi2stm/acsi2stm.h

# Remove previous build artifacts

rm -rf build acsi2stm-$VERSION
mkdir build
mkdir acsi2stm-$VERSION

cd build

echo Compile testacsi

vasmm68k_mot -devpac -ldots -showopt -Ftos -L testacsi.lst -o ../acsi2stm-$VERSION/testacsi.tos ../testacsi/testacsi.s

[ -e ../acsi2stm-$VERSION/testacsi.tos ] || exit $?
grep 'There have been no errors.' testacsi.lst &>/dev/null || exit 1

echo Compile the TOS loader

vasmm68k_mot -devpac -ldots -showopt -Ftos -L stmdrive.lst -o ../acsi2stm-$VERSION/stmdrive.tos ../stmdrive/stmdrive.s

[ -e ../acsi2stm-$VERSION/stmdrive.tos ] || exit $?
grep 'There have been no errors.' stmdrive.lst &>/dev/null || exit 1

echo Compile the boot sector

vasmm68k_mot -devpac -ldots -showopt -Fbin -L boot.lst -o boot.bin ../stmdrive/boot.s

[ -e boot.bin ] || exit $?
grep 'There have been no errors.' boot.lst &>/dev/null || exit 1

echo Generate source code from the binary blob

xxd -i boot.bin > ../acsi2stm/boot.h

echo Compile the arduino binary

arduino --pref build.path=./ --preserve-temp-files --verify ../acsi2stm/acsi2stm.ino

[ -e acsi2stm.ino.bin ] || exit $?

cp acsi2stm.ino.bin ../acsi2stm-$VERSION

echo ... and the legal stuff

cp ../LICENSE ../acsi2stm-$VERSION

cat > ../acsi2stm-$VERSION/README.txt << EOF
ACSI2STM Atari hard drive emulator
Copyright (C) 2019-2021 by Jean-Matthieu Coulon

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

For source code, documentation and more information about this
software, see its GitHub page:

    https://github.com/retro16/acsi2stm

EOF

echo Create release zip package

cd ..
zip -r acsi2stm-$VERSION.zip acsi2stm-$VERSION

echo Clean intermediate files

rm -r build
rm -r acsi2stm-$VERSION
