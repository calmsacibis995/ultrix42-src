#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	paste

OBJS=	paste.o

paste:	paste.o
paste.o:	paste.c

install:
	$(INSTALL) -c -s paste $(DESTROOT)/usr/bin/paste

include $(GMAKERULES)
