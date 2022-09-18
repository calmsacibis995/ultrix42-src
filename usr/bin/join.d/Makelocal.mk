#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	join

OBJS=	join.o

LOADLIBES=

join:	join.o
join.o:	join.c

install:
	$(INSTALL) -c -s join $(DESTROOT)/usr/bin/join

include $(GMAKERULES)
