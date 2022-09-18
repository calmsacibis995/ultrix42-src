#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	split

OBJS=	split.o

split:	split.o
split.o:	split.c

install:
	$(INSTALL) -c -s split $(DESTROOT)/usr/bin/split

include $(GMAKERULES)
