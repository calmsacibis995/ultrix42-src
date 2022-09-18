#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	flcopy

OBJS=	flcopy.o

flcopy.o:	flcopy.c

install:
	$(INSTALL) -c -s flcopy $(DESTROOT)/usr/etc/flcopy
	$(RM) $(DESTROOT)/etc/flcopy
	$(LN) -s ../usr/etc/flcopy $(DESTROOT)/etc/flcopy

include $(GMAKERULES)
