#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

CFLAGS= -O -Y

AOUT=	nl

OBJS=	nl.o

nl:	nl.o
nl.o:	nl.c

install:
	$(INSTALL) -c -s nl $(DESTROOT)/usr/bin/nl

include $(GMAKERULES)
