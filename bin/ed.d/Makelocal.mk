#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

CDEFINES=-DEIGHTBIT

AOUT=	ed

OBJS=	ed.o

include ../Makelocal_$(MACHINE).mk

ed.o:	ed.c

install:
	$(INSTALL) -c -s -m 755 ed $(DESTROOT)/bin/ed
	$(RM) $(DESTROOT)/bin/red $(DESTROOT)/bin/e
	$(LN) $(DESTROOT)/bin/ed $(DESTROOT)/bin/red
	$(LN) $(DESTROOT)/bin/ed $(DESTROOT)/bin/e

include $(GMAKERULES)
