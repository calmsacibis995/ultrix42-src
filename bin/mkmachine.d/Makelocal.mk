#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	mkmachine

OBJS=	mkmachine.o

mkmachine.o:	mkmachine.c

all:		machine

machine:	mkmachine
	mkmachine >machine
	$(CHMOD) +x machine

install:
	$(INSTALL) -c -m 755 machine $(DESTROOT)/bin/machine

include $(GMAKERULES)
