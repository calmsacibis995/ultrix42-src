#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

CFLAGS=	-O

LOADLIBES=-li

AOUT=	iconv

OBJS=	iconv.o

iconv:	iconv.o
iconv.o:	iconv.c

install:
	$(INSTALL) -o root -g system -m 755 -c -s iconv $(DESTROOT)/usr/bin/iconv

include $(GMAKERULES)
