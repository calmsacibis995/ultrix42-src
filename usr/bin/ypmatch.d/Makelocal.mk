#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	ypmatch

OBJS=	ypmatch.o

ypmatch:	ypmatch.o
ypmatch.o:	ypmatch.c

install:
	$(INSTALL) -c -s ypmatch $(DESTROOT)/usr/bin/ypmatch

include $(GMAKERULES)
