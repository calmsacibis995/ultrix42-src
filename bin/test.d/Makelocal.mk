#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	test

OBJS=	test.o

test.o:	test.c

install:
	$(INSTALL) -c -s test $(DESTROOT)/bin/test
	$(RM) $(DESTROOT)/bin/[
	$(LN) $(DESTROOT)/bin/test $(DESTROOT)/bin/[

include $(GMAKERULES)
