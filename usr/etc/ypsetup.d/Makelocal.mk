#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

install:
	$(INSTALL) -c -m 744 ../ypsetup.sh $(DESTROOT)/usr/etc/ypsetup
	$(RM) $(DESTROOT)/etc/ypsetup
	$(LN) -s ../usr/etc/ypsetup $(DESTROOT)/etc/ypsetup

include $(GMAKERULES)
