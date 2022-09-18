#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	spline

OBJS=	spline.o

spline:	spline.o
spline.o:	spline.c

install:
	$(INSTALL) -c -s spline $(DESTROOT)/usr/bin/spline

include $(GMAKERULES)
