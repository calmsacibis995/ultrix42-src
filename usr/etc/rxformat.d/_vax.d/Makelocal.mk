#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=rxformat

OBJS=rxformat.o

rxformat.o:	rxformat.c

install:
	$(INSTALL) -c rxformat $(DESTROOT)/usr/etc/rxformat
	$(RM) $(DESTROOT)/etc/rxformat
	$(LN) -s ../usr/etc/rxformat $(DESTROOT)/etc/rxformat

include $(GMAKERULES)
