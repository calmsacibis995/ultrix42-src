#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	ptx

OBJS=	ptx.o

ptx:	ptx.o
ptx.o:	ptx.c

install:
	$(INSTALL) -c -s ptx $(DESTROOT)/usr/bin/ptx

include $(GMAKERULES)
