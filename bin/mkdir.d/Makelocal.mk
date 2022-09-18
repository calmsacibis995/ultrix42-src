#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	mkdir

OBJS=	mkdir.o

mkdir.o:	mkdir.c

install:
	$(INSTALL) -c -s mkdir $(DESTROOT)/bin/mkdir

include $(GMAKERULES)
