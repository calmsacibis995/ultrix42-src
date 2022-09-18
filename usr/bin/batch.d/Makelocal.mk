#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	batch

OBJS=	batch.o

batch:	batch.o
batch.o:	batch.c

install:
	$(INSTALL) -c -s batch $(DESTROOT)/usr/bin/batch

include $(GMAKERULES)
