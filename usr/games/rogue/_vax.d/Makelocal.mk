#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
include $(GMAKEVARS)

CFLAGS=	-O -DSCOREFILE='"/usr/games/lib/rogue_roll"' \
	-DNAMELIST='"/dev/vmunix"' -DLOADAV -DMAXLOAD=50

all: rogue

rogue: mach_dep.o distmod.obj
	$(CC) -o rogue ../distmod.obj mach_dep.o  -lcurses -ltermlib

mach_dep.o:
	cc ${CFLAGS} -c ../mach_dep.c

install:
	$(INSTALL) -c -o daemon -s -m 4711 rogue \
		${DESTROOT}/usr/games/rogue
	$(INSTALL) -c -o daemon -m 644 /dev/null \
		${DESTROOT}/usr/games/lib/rogue_roll


include $(GMAKERULES)
