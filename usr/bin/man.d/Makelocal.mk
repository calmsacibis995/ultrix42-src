#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

install:
	$(INSTALL) -c ../man.sh $(DESTROOT)/usr/bin/man

include $(GMAKERULES)
