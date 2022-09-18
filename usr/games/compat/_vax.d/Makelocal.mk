#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

all:	v7run

OBJS=	compat.o v7run.o v7start.o v7traps.o unixemts.o rdosig.o \
	dofloat.o bcopy.o

v7run:	$(OBJS)
	ld -e start -N -o v7run $(OBJS) /lib/crt0.o -lc -s

compat.o: memsiz.s compat.s
	cat ../memsiz.s ../compat.s | as -o compat.o

v7run.o: defs.h ../unixhdr.h ../runcompat.c
	cc -c -O -DV7UNIX -DUNIX ../runcompat.c
	mv runcompat.o v7run.o

bcopy.o: bcopy.c

v7start.o: defs.h ../unixstart.c
	cc -c -O -DV7UNIX ../unixstart.c
	mv unixstart.o v7start.o

v7traps.o: defs.h ../unix7sys.h ../unixtraps.c
	cc -c -O -DV7UNIX ../unixtraps.c
	mv unixtraps.o v7traps.o

unixemts.o: unixemts.c
	cc -c -O ../unixemts.c

rdosig.o: dosig.c
	cc -c -O ../dosig.c
	mv dosig.o rdosig.o

dofloat.o: defs.h ../dofloat.c
	cc -c -O ../dofloat.c

install:
	install -c v7run $(DESTROOT)/usr/games/lib/compat

include $(GMAKERULES)
