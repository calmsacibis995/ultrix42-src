#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	tsort

OBJS=	tsort.o

tsort:	tsort.o
tsort.o:	tsort.c

install:
	$(INSTALL) -c -s tsort $(DESTROOT)/usr/bin/tsort

include $(GMAKERULES)
