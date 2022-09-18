#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib/learn/vi

install:
	$(CP) ../L* ../Init ../longtext $(DESTROOT)/usr/lib/learn/vi
	$(INSTALL) -c -m 644 ../Makefile.mk $(DESTROOT)/usr/lib/learn/vi/Makefile
	$(INSTALL) -c -m 644 ../README $(DESTROOT)/usr/lib/learn/vi/README
	$(INSTALL) -c -m 644 ../longtext $(DESTROOT)/usr/lib/learn/vi/longtext

include $(GMAKERULES)
