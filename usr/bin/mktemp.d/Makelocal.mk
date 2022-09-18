#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/bin

AOUT=	mktemp

OBJS=	mktemp.o

mktemp.o:	mktemp.c

install:
	$(INSTALL) -c -s mktemp $(DESTROOT)/usr/bin/mktemp

include $(GMAKERULES)
