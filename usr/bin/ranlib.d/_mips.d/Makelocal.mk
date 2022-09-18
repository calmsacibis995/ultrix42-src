#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

install:
	$(INSTALL) -c -m 755 ../ranlib.sh ${DESTROOT}/usr/bin/ranlib

include $(GMAKERULES)
