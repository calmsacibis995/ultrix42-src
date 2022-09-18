#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

install:
	$(INSTALL) -c -m 755 -o root -g system ../lk ${DESTROOT}/usr/bin/lk
include $(GMAKERULES)
