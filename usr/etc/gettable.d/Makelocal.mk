#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	gettable

OBJS=	gettable.o

gettable.o:	gettable.c

install:
	$(INSTALL) -c -s gettable $(DESTROOT)/usr/etc/gettable
	$(RM) $(DESTROOT)/etc/gettable
	$(LN) -s ../usr/etc/gettable $(DESTROOT)/etc/gettable

include $(GMAKERULES)
