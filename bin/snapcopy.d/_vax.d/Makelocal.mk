#  @(#)Makelocal.mk	2.1  ULTRIX  4/20/89

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

LOADLIBES=-lerrlog

AOUT=	snapcopy

OBJS=	snapcopy.o

snapcopy.o:	snapcopy.c

install:
	$(INSTALL) -c -s snapcopy $(DESTROOT)/bin/snapcopy
	$(RM) $(DESTROOT)/etc/snapcopy
	$(LN) -s ../bin/snapcopy $(DESTROOT)/etc/snapcopy

include $(GMAKERULES)
