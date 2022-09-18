#@(#)Makefile	4.1	Ultrix	7/17/90
# $Header: Makefile,v 1.5 84/05/19 11:41:50 powell Exp $

DESTROOT=
INSTALLDIR = ${DESTROOT}/usr/bin
MODLIBDIR = ${DESTROOT}/usr/lib/mod
CFLAGS = #-g	

imod:	mod.c Makefile
	cc -o imod -DMODLIBDIR='"${MODLIBDIR}"' ${CFLAGS} mod.c
install:	imod
	cp imod $(INSTALLDIR)/mod
clean:
	-rm -f *.o mod imod
