#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

CFLAGS=	-O -Y

LOADLIBES=-li

AOUT=	sort5

OBJS=	sort5.o

sort5:	sort5.o
sort5.o:	sort5.c

install:
	$(INSTALL) -c -s sort5 $(DESTROOT)/usr/lib/sort5

include $(GMAKERULES)
