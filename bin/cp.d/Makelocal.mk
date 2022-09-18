#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	cp

OBJS=	cp.o

cp.o:	cp.c

#	take care with cp; install uses it!

install:
	$(INSTALL) -c -s cp $(DESTROOT)/bin/newcp; \
		$(MV) -f $(DESTROOT)/bin/newcp $(DESTROOT)/bin/cp;
		
include $(GMAKERULES)
