#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	mesg

OBJS=	mesg.o

mesg:	mesg.o
mesg.o:	mesg.c

install:
	$(INSTALL) -c -s mesg $(DESTROOT)/usr/bin/mesg

include $(GMAKERULES)
