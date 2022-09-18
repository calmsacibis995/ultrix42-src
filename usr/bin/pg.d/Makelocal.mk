#  @(#)Makelocal.mk	4.2  ULTRIX  11/14/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/bin

LOADLIBES=-lcursesX

AOUT=	pg

OBJS=	pg.o

pg:	pg.o
pg.o:	pg.c

install:
	$(INSTALL) -c -s pg $(DESTROOT)/usr/bin/pg

include $(GMAKERULES)
