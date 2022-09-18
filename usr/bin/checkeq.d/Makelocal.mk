#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	checkeq

OBJS=	checkeq.o

checkeq:	checkeq.o
checkeq.o:	checkeq.c

install:
	$(INSTALL) -c -s checkeq $(DESTROOT)/usr/bin/checkeq

include $(GMAKERULES)
