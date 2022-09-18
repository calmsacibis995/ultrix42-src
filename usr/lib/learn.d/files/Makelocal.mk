#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib/learn/files

install:
	$(CP) ../L* $(DESTROOT)/usr/lib/learn/files
	$(INSTALL) -c -m 755 ../Init $(DESTROOT)/usr/lib/learn/files/Init

include $(GMAKERULES)
