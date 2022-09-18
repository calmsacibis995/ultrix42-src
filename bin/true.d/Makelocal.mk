#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

install:
	$(INSTALL) -c ../true.sh $(DESTROOT)/bin/true

include $(GMAKERULES)
