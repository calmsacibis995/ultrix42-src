#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	dd

OBJS=	dd.o

dd.o:	dd.c

install:
	$(INSTALL) -c -s dd $(DESTROOT)/bin/dd

include $(GMAKERULES)
