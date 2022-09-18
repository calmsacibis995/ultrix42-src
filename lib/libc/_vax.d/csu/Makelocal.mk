#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	crt0.o mcrt0.o gcrt0.o mon.o gmon.o
DFLMON= mon.o

all: $(OBJS)

install:
	$(INSTALL) -c -m 644 crt0.o $(DESTROOT)/usr/lib/crt0.o
	$(INSTALL) -c -m 644 mcrt0.o $(DESTROOT)/usr/lib/mcrt0.o
	$(INSTALL) -c -m 644 gcrt0.o $(DESTROOT)/usr/lib/gcrt0.o
	$(INSTALL) -c -m 644 $(DFLMON) $(DESTROOT)/usr/lib/$(DFLMON)

crt0.o:	crt0.c
	cc -S $(DFLAGS) -DCRT0 ../crt0.c
	$(CPP) crt0.s > x.s
	$(AS) -o x.o x.s
	ld -x -r -o crt0.o x.o
	$(RM) x.s x.o crt0.s

moncrt0.o: crt0.c
	cc -S $(DFLAGS) -DMCRT0 ../crt0.c
	$(CPP) crt0.s > x.s
	as -o x.o x.s
	ld -x -r -o moncrt0.o x.o
	$(RM) x.s x.o crt0.s

gcrt0.o: moncrt0.o gmon.o
	ld -x -r -o gcrt0.o moncrt0.o gmon.o

mcrt0.o: moncrt0.o mon.o
	ld -x -r -o mcrt0.o moncrt0.o mon.o

mon.o: mon.c mon.ex
	cc -S $(DFLAGS) ../mon.c
	ex - mon.s < ../mon.ex
	$(AS) -o x.o mon.s
	ld -x -r -o mon.o x.o
	$(RM) x.o mon.s

gmon.o: gmon.c gmon.h gmon.ex
	cc -S $(DFLAGS) ../gmon.c
	ex - gmon.s < ../gmon.ex
	$(AS) -o x.o gmon.s
	ld -x -r -o gmon.o x.o
	$(RM) x.o gmon.s

include $(GMAKERULES)
