#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/bin

AOUT=	mkfifo

OBJS=	mkfifo.o

mkfifo.o:	mkfifo.c

install:
	$(INSTALL) -c -s mkfifo $(DESTROOT)/usr/bin/mkfifo

include $(GMAKERULES)
