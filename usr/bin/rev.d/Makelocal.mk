#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	rev

OBJS=	rev.o

rev:	rev.o
rev.o:	rev.c

install:
	$(INSTALL) -c -s rev $(DESTROOT)/usr/bin/rev

include $(GMAKERULES)
