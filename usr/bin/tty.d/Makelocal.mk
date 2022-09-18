#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	tty

OBJS=	tty.o

tty:	tty.o
tty.o:	tty.c

install:
	$(INSTALL) -c -s tty $(DESTROOT)/usr/bin/tty

include $(GMAKERULES)
