#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	date

LOADLIBES=	-li

OBJS=	date.o

date.o:	date.c

install:
	$(INSTALL) -c -s date $(DESTROOT)/bin/date

include $(GMAKERULES)
