#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	ls

OBJS=	ls.o

ls.o:	ls.c

install:
	$(INSTALL) -c -s ls $(DESTROOT)/bin/ls

include $(GMAKERULES)
