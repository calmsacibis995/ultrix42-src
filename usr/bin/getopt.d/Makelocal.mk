#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

CFLAGS= -O -Y

AOUT=	getopt

OBJS=	getopt.o

getopt:	getopt.o
getopt.o:	getopt.c

install:
	$(INSTALL) -c -s getopt $(DESTROOT)/usr/bin/getopt

include $(GMAKERULES)
