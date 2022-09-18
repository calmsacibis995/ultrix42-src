#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

install:
	$(INSTALL) -c ../diffmk.sh $(DESTROOT)/usr/bin/diffmk

include $(GMAKERULES)
