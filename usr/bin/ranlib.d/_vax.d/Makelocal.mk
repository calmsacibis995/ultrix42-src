#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	ranlib

OBJS=	ranlib.o

ranlib.o:	ranlib.c

install:
	$(INSTALL) -s -c ranlib $(DESTROOT)/usr/bin/ranlib

include $(GMAKERULES)
