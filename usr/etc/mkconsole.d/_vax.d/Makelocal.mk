#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

install:
	$(INSTALL) -c -m 744 ../mkconsole.sh $(DESTROOT)/usr/etc/mkconsole
	$(RM) $(DESTROOT)/etc/mkconsole
	$(LN) -s ../usr/etc/mkconsole $(DESTROOT)/etc/mkconsole

include $(GMAKERULES)
