#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	pwd

OBJS=	pwd.o

pwd.o:	pwd.c

install:
	$(INSTALL) -c -s pwd $(DESTROOT)/bin/pwd

include $(GMAKERULES)
