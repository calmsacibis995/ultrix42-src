#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/bin

AOUT=	fgrep

OBJS=	fgrep.o

fgrep:	fgrep.o
fgrep.o:	fgrep.c

install:
	$(INSTALL) -c -s fgrep $(DESTROOT)/usr/bin/fgrep

include $(GMAKERULES)
