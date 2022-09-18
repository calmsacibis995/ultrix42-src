#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	stty

OBJS=	stty.o

stty.o:	stty.c

install:
	$(INSTALL) -c -s stty $(DESTROOT)/bin/stty

include $(GMAKERULES)
