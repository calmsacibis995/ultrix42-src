#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	chmod

OBJS=	chmod.o

chmod.o:	chmod.c

install:
	$(INSTALL) -c -s chmod $(DESTROOT)/bin/chmod

include $(GMAKERULES)
