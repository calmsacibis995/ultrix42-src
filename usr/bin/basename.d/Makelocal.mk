#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

install:
	$(INSTALL) -c ../basename.sh $(DESTROOT)/usr/bin/basename

include $(GMAKERULES)
