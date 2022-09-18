#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	grep

OBJS=	grep.o

grep.o:	grep.c

install:
	$(INSTALL) -c -s grep $(DESTROOT)/bin/grep

include $(GMAKERULES)
