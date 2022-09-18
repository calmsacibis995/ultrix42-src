#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

install:
	$(INSTALL) -c ../lorder.sh $(DESTROOT)/usr/bin/lorder

include $(GMAKERULES)
