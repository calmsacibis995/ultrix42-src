#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	ps
OBJS=	ps.o
LOADLIBES=-lm

ps.o:	ps.c

install:
	$(INSTALL) -c -m 2711 -g kmem -s ps $(DESTROOT)/bin/ps

include $(GMAKERULES)
