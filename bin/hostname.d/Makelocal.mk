#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	hostname

OBJS=	hostname.o

hostname.o:	hostname.c

install:
	$(INSTALL) -c -s hostname $(DESTROOT)/bin/hostname

include $(GMAKERULES)
