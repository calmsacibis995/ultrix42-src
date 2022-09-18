#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	rm

OBJS=	rm.o

rm.o:	rm.c

install:
	$(INSTALL) -c -s rm $(DESTROOT)/bin/rm

include $(GMAKERULES)
