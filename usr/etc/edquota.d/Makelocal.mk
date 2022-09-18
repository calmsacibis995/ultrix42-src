#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	edquota

OBJS=	edquota.o

edquota.o:	edquota.c

install:
	$(INSTALL) -c -s edquota $(DESTROOT)/usr/etc/edquota
	$(RM) $(DESTROOT)/etc/edquota
	$(LN) -s ../usr/etc/edquota $(DESTROOT)/etc/edquota

include $(GMAKERULES)
