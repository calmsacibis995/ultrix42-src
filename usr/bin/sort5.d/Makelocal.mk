#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

CFLAGS=	-O -Y

LOADLIBES=-li

AOUT=	sort5

OBJS=	sort5.o

sort5:	sort5.o
sort5.o:	sort5.c

install:
	$(INSTALL) -o root -g system -m 755 -c -s sort5 $(DESTROOT)/usr/bin/sort5

include $(GMAKERULES)
