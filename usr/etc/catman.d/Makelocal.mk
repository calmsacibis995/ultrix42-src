#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	catman

OBJS=	catman.o

catman.o:	catman.c

install:
	$(INSTALL) -c -s catman $(DESTROOT)/usr/etc/catman
	$(RM) $(DESTROOT)/etc/catman
	$(LN) -s ../usr/etc/catman $(DESTROOT)/etc/catman

include $(GMAKERULES)
