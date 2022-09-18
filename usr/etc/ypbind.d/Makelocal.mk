#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	ypbind

OBJS=	ypbind.o

ypbind.o:	ypbind.c

install:
	$(INSTALL) -c -s ypbind $(DESTROOT)/usr/etc/ypbind
	$(RM) $(DESTROOT)/etc/ypbind
	$(LN) -s ../usr/etc/ypbind $(DESTROOT)/etc/ypbind

include $(GMAKERULES)
