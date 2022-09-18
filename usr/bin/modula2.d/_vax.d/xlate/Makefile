#@(#)Makefile	4.1	Ultrix	7/17/90
# $Header: Makefile,v 1.4 84/05/19 11:32:14 powell Exp $

DESTROOT=
MODLIBDIR = ${DESTROOT}/usr/lib/mod
PFLAGS = -w #-g
H =	consts.h types.h vars.h error.h ees.h output.h machine.h
OBJS =	codesubs.o ees.o error.o init.o input.o iolib.o machine.o memref.o \
	opsubs.o output.o util.o xlate.o
xlate:	$(OBJS)
	pc ${PFLAGS} -o xlate $(OBJS)
xlate.o:	xlate.p opsubs.h codesubs.h input.h init.h util.h $(H)
opsubs.o:	opsubs.p opsubs.h codesubs.h memref.h util.h $(H)
codesubs.o:	codesubs.p codesubs.h memref.h util.h $(H)
init.o:		init.p init.h readch.i $(H)
input.o:	input.p input.h util.h readch.i $(H)
output.o:	output.p $(H)
ees.o:		ees.p memref.h $(H)
util.o:		util.p util.h $(H)
error.o:	error.p $(H)
machine.o:	machine.p $(H)
memref.o:	memref.p memref.h util.h codesubs.h $(H)
iolib.o:	iolib.c
install:	xlate
	cp xlate $(MODLIBDIR)/mod2.1
clean:
	-rm -f *.o
