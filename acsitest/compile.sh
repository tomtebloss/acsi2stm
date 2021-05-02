rm -f acsitest.lst acsitest.tos
vasmm68k_mot -devpac -showopt -Ftos -L acsitest.lst -o acsitest.tos acsitest.s
