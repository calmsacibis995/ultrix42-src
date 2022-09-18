#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	sort

OBJS=	sort.o

sort:	sort.o
sort.o:	sort.c

install:
	$(INSTALL) -c -s sort $(DESTROOT)/usr/bin/sort

include $(GMAKERULES)
