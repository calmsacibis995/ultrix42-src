#@(#)Makefile	4.1	Ultrix	7/17/90
# $Header: Makefile,v 1.5 84/05/19 11:39:34 powell Exp $

.SUFFIXES:
.SUFFIXES: .o .mod .def .c

CFLAGS = #-g
MFLAGS = #-g -C

DESTROOT=
MODLIBDIR = ${DESTROOT}/usr/lib/mod

DEFS = Storage.def math.def parameters.def strings.def unix.def
PDEFS = system.def memory.def io.def bitoperations.def
OBJS = runtime.o mem.o udiv.o coroutine.o
MOBJS =	math.o parameters.o strings.o unix.o

.def.mod:
	touch $*.mod
.mod.o:
	mod -c -pg ${MFLAGS} $*.mod
	mv $*.o prof
	mod -c ${MFLAGS} $*.mod
.c.o:
	cc -c -pg ${CFLAGS} $*.c
	mv $*.o prof
	cc -c ${CFLAGS} $*.c

all :	modlib modlib_p

modlib:	$(OBJS) $(MOBJS)
	-rm -f modlib
	ar cr modlib $(OBJS) $(MOBJS)
	ranlib modlib
modlib_p: $(OBJS) $(MOBJS)
	-rm -f prof/modlib_p modlib_p
	cd prof; ar cr modlib_p $(OBJS) $(MOBJS)
	ln prof/modlib_p
	ranlib modlib_p
udiv.o:	udiv.c
	as -o udiv.o udiv.c
	cp udiv.o prof
coroutine.o:	coroutine.c
	as -o coroutine.o coroutine.c
	cp coroutine.o prof
clean:
	-rm -f *.s *.pcd *.o prof/*.o
install: modlib modlib_p
	(cd $(MODLIBDIR); rm -f modlib modlib_p $(DEFS) $(PDEFS))
	cp modlib modlib_p $(DEFS) $(PDEFS) $(MODLIBDIR)
	ranlib $(MODLIBDIR)/modlib
	ranlib $(MODLIBDIR)/modlib_p
testdir:
	ln -s ../mod2/mod2 mod2.0
	ln -s ../xlate/xlate mod2.1
	ln -s ../imc/imc mod2.2
