#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	tr

OBJS=	tr.o

tr:	tr.o
tr.o:	tr.c

install:
	$(INSTALL) -c -s tr $(DESTROOT)/usr/bin/tr

include $(GMAKERULES)
