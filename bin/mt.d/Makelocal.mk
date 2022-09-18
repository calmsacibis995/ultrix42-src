#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	mt

OBJS=	mt.o

mt.o:	mt.c

install:
	$(INSTALL) -c -s mt $(DESTROOT)/bin/mt

include $(GMAKERULES)
