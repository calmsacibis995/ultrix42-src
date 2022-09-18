#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	cal

OBJS=	cal.o

cal:	cal.o
cal.o:	cal.c

install:
	$(INSTALL) -c -s cal $(DESTROOT)/usr/bin/cal

include $(GMAKERULES)
