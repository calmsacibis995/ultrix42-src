#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

install:
	$(INSTALL) -c -m 744 ../snmpsetup.sh $(DESTROOT)/usr/etc/snmpsetup

include $(GMAKERULES)
