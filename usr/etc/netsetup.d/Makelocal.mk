#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

CDEFINES=-DCONFDIR=\"/sys/conf/$(MACHINE)\"

AOUT=	netsetup

OBJS=	netsetup.o

netsetup.o:	netsetup.c

install:
	$(INSTALL) -c -s netsetup $(DESTROOT)/usr/etc/netsetup
	$(RM) $(DESTROOT)/etc/netsetup
	$(LN) -s ../usr/etc/netsetup $(DESTROOT)/etc/netsetup

include $(GMAKERULES)
