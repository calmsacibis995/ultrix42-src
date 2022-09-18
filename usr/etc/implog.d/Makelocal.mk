#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUTS=	implog implogd

implog:		implog.o
implog.o:	implog.c

implogd:	implogd.o
implogd.o:	implogd.c

install:
	$(INSTALL) -c -s implog $(DESTROOT)/usr/etc/implog
	$(RM) $(DESTROOT)/etc/implog
	$(LN) -s ../usr/etc/implog $(DESTROOT)/etc/implog
	$(INSTALL) -c -s implogd $(DESTROOT)/usr/etc/implogd
	$(RM) $(DESTROOT)/etc/implogd
	$(LN) -s ../usr/etc/implogd $(DESTROOT)/etc/implogd

include $(GMAKERULES)
