#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

LIBOBJS =fortrtl.a fortlib.a fortinitial.o fortmsgfile

SUBDIRS = omdir

install:
	$(INSTALL) -c -m 755 -o root -g system ../omdir/_$(MACHINE).b/om ${DESTROOT}/usr/bin/om
	@for i in ${LIBOBJS}; \
	do \
		$(ECHO) "$(INSTALL) -c -m 644 -o root -g system ../$$i ${DESTROOT}/usr/lib/$$i"; \
		$(INSTALL) -c -m 644 -o root -g system ../$$i ${DESTROOT}/usr/lib/$$i; \
	done

include $(GMAKERULES)
