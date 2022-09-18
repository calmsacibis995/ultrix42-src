#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/bin

AOUT=	find

OBJS=	find.o

find:	find.o
find.o:	find.c

install:
	$(INSTALL) -c -s find $(DESTROOT)/usr/bin/find

include $(GMAKERULES)
