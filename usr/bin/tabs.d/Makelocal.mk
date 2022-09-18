#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	tabs

OBJS=	tabs.o

tabs:	tabs.o
tabs.o:	tabs.c

install:
	$(INSTALL) -c -s tabs $(DESTROOT)/usr/bin/tabs

include $(GMAKERULES)
