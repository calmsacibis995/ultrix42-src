#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	cb

OBJS=	cb.o

cb:	cb.o
cb.o:	cb.c

install:
	$(INSTALL) -c -s cb $(DESTROOT)/usr/bin/cb

include $(GMAKERULES)
