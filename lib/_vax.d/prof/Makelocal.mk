#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

CDEFINES=-Dplot
LOADLIBES=-lplot

AOUT=	prof

OBJS=	prof.o

prof.o:	prof.c

install:
	$(INSTALL) -c -s prof $(DESTROOT)/usr/bin/prof

include $(GMAKERULES)
