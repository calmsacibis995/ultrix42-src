#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	cat

OBJS=	cat.o

cat.o:	cat.c

install:
	$(INSTALL) -c -s cat $(DESTROOT)/bin/cat

include $(GMAKERULES)
