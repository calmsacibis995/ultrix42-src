#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	look

OBJS=	look.o

look:	look.o
look.o:	look.c

install:
	$(INSTALL) -c -s look $(DESTROOT)/usr/bin/look

include $(GMAKERULES)
