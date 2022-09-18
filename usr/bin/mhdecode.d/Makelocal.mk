#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	mhdecode

OBJS=	mhdecode.o

mhdecode:	mhdecode.o
mhdecode.o:	mhdecode.c

install:
	$(INSTALL) -c -s mhdecode $(DESTROOT)/usr/bin/mhdecode

include $(GMAKERULES)
