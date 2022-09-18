#  @(#)Makelocal.mk	4.1	ULTRIX	7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/bin/

AOUT=	pfstat

OBJS=	pfstat.o

pfstat.o:	pfstat.c

install:
	$(INSTALL) -c -s -m 2755 -g kmem pfstat $(DESTROOT)/usr/bin/pfstat

include $(GMAKERULES)
