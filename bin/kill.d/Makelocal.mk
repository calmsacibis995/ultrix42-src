#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	kill

OBJS=	kill.o

kill.o:	kill.c

install:
	$(INSTALL) -c -s kill $(DESTROOT)/bin/kill

include $(GMAKERULES)
