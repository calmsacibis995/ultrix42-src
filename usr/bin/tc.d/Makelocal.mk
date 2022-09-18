#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	tc

OBJS=	tc.o

tc:	tc.o
tc.o:	tc.c

install:
	$(INSTALL) -c -s tc $(DESTROOT)/usr/bin/tc

include $(GMAKERULES)
