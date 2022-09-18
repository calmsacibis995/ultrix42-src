#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	sum

OBJS=	sum.o

sum:	sum.o
sum.o:	sum.c

install:
	$(INSTALL) -c -s sum $(DESTROOT)/usr/bin/sum

include $(GMAKERULES)
