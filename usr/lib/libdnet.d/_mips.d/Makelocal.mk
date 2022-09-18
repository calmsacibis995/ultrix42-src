#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(SRCROOT)/usr/lib

install:
	$(INSTALL) -c -m 644 ../libdnet.a $(DESTROOT)/usr/lib/libdnet.a
	$(RANLIB) $(DESTROOT)/usr/lib/libdnet.a

include $(GMAKERULES)
