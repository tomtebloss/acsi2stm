rm -f testacsi.lst testacsi.tos
vasmm68k_mot -devpac -showopt -Ftos -L testacsi.lst -o testacsi.tos testacsi.s
