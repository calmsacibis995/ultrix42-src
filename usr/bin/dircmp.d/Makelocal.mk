#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

install:
	$(INSTALL) -c ../dircmp.sh $(DESTROOT)/usr/bin/dircmp

include $(GMAKERULES)
