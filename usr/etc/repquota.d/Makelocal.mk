#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	repquota

OBJS=	repquota.o

repquota.o:	repquota.c

install:
	$(INSTALL) -c -s repquota $(DESTROOT)/usr/etc/repquota
	$(RM) $(DESTROOT)/etc/repquota
	$(LN) -s ../usr/etc/repquota $(DESTROOT)/etc/repquota

include $(GMAKERULES)
