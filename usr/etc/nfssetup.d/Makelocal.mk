#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

install:
	$(INSTALL) -c -m 744 ../nfssetup.sh $(DESTROOT)/usr/etc/nfssetup
	$(RM) $(DESTROOT)/etc/nfssetup
	$(LN) -s ../usr/etc/nfssetup $(DESTROOT)/etc/nfssetup

include $(GMAKERULES)
