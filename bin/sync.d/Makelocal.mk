#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	sync

OBJS=	sync.o

sync.o:	sync.c

install:
	$(INSTALL) -c -s sync $(DESTROOT)/bin/sync

include $(GMAKERULES)
