#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib/cda

install: 
	$(INSTALL) -c -m 0644 ../defstyle.ddif $(DESTROOT)/usr/lib/cda/defstyle.ddif

include $(GMAKERULES)
