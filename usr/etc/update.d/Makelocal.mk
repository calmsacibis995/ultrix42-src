#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	update

OBJS=	update.o

update.o:	update.c

install:
	$(INSTALL) -c -s update $(DESTROOT)/usr/etc/update
	$(RM) $(DESTROOT)/etc/update
	$(LN) -s ../usr/etc/update $(DESTROOT)/etc/update

include $(GMAKERULES)
