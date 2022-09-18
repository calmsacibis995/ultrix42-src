#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib/learn/eqn

install:
	$(CP) ../L* $(DESTROOT)/usr/lib/learn/eqn
	$(INSTALL) -c -m 755 ../Init $(DESTROOT)/usr/lib/learn/eqn/Init
	$(INSTALL) -c ../tinyms $(DESTROOT)/usr/lib/learn/eqn/tinyms

include $(GMAKERULES)
