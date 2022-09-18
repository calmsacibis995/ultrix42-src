#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

install:
	$(INSTALL) -c ../nohup.sh $(DESTROOT)/usr/bin/nohup

include $(GMAKERULES)
