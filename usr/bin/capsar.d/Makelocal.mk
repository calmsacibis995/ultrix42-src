#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

LOADLIBES=-lcapsar

AOUT=	capsar

OBJS=	capsar.o

capsar:	capsar.o
capsar.o:	capsar.c

install:
	$(INSTALL) -c -s capsar $(DESTROOT)/usr/bin/capsar

include $(GMAKERULES)
