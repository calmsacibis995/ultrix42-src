#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	ypwhich

OBJS=	ypwhich.o

ypwhich:	ypwhich.o
ypwhich.o:	ypwhich.c

install:
	$(INSTALL) -c -s ypwhich $(DESTROOT)/usr/bin/ypwhich

include $(GMAKERULES)
