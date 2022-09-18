#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

install:
	$(INSTALL) -c -m 755 -o root -g system ../vcc ${DESTROOT}/usr/bin/vcc
	$(INSTALL) -c -m 755 -o root -g system ../vaxc ${DESTROOT}/usr/lib/vaxc
	$(INSTALL) -c -m 755 -o root -g system ../cerrfile ${DESTROOT}/usr/lib/cerrfile

include $(GMAKERULES)
