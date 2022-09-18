#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)


AOUT=	fsplit
OBJS=	fsplit.o

fsplit.o:	fsplit.c

install:
		$(INSTALL) -c -s fsplit ${DESTROOT}/usr/ucb/fsplit

include $(GMAKERULES)
