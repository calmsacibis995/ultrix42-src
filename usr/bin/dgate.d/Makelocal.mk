#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	dgate

OBJS=	dgate.o subr.o

dgate:		dgate.o

dgate.o:	dgate.c dgate.h
subr.o:		subr.c dgate.h

install:
	$(INSTALL) -c -s -m 4755 dgate $(DESTROOT)/usr/bin/dgate

include $(GMAKERULES)
