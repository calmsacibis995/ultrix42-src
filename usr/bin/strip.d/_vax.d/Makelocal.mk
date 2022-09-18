# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)
AOUT=strip
OBJS=strip.o
CFLAGS=-O
LDFLAGS= -s

pretools tools1 tools2: all
pretools tools1 tools2 install:
	install -c strip ${DESTROOT}/usr/bin/strip
	$(RM) ${DESTROOT}/bin/strip
	ln -s ../usr/bin/strip ${DESTROOT}/bin/strip

strip.o:	strip.c

include $(GMAKERULES)
