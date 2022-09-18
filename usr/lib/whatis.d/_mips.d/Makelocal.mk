# @(#)Makelocal.mk	1.1	(ULTRIX)	3/16/89

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib

install: 

	$(INSTALL) -c -m 644 -g system ../whatis $(DESTROOT)/usr/lib/whatis

include $(GMAKERULES)
