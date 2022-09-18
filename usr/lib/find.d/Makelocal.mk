#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTDIR=$(DESTROOT)/usr/lib/find

DESTLIST=$(DESTDIR)

FILES = bigram code updatedb

install:
	@for i in $(FILES); do \
		$(ECHO) "$(INSTALL) -c ../$$i $(DESTDIR)/$$i"; \
		$(INSTALL) -c ../$$i $(DESTDIR)/$$i; \
	done

include $(GMAKERULES)
