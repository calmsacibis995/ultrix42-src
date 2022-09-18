#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	ypcat

OBJS=	ypcat.o

ypcat:	ypcat.o
ypcat.o:	ypcat.c

install:
	$(INSTALL) -c -s ypcat $(DESTROOT)/usr/bin/ypcat

include $(GMAKERULES)
