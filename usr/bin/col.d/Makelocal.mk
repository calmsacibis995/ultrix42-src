#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	col

OBJS=	col.o

col:	col.o
col.o:	col.c

install:
	$(INSTALL) -c -s col $(DESTROOT)/usr/bin/col

include $(GMAKERULES)
