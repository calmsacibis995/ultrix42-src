#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	xargs

OBJS=	xargs.o

xargs:	xargs.o
xargs.o:	xargs.c

install:
	$(INSTALL) -c -s xargs $(DESTROOT)/usr/bin/xargs

include $(GMAKERULES)
