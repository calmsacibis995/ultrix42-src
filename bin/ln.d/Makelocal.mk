#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	ln

OBJS=	ln.o

ln.o:	ln.c

install:
	$(INSTALL) -c -s ln $(DESTROOT)/bin/ln

include $(GMAKERULES)
