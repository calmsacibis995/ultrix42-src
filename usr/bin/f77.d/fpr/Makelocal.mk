#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	fpr

OBJS=	fpr.o

fpr.o:	fpr.c

install:
		$(INSTALL) -c -s fpr $(DESTROOT)/usr/ucb/fpr

include $(GMAKERULES)
