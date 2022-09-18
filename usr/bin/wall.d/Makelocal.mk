#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	wall

OBJS=	wall.o

wall:	wall.o
wall.o:	wall.c

install:
	$(INSTALL) -c -g tty -m 2755 -s wall $(DESTROOT)/usr/bin/wall
	$(RM) $(DESTROOT)/bin/wall
	$(LN) -s ../usr/bin/wall $(DESTROOT)/bin/wall

include $(GMAKERULES)
