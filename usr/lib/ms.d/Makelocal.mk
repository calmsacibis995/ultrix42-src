#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTDIR=$(DESTROOT)/usr/lib/ms
FILES= README end.awk endnote swapacc
SFILES= acc cov eqn ref tbl ths toc

install:
	-@if [ ! -d $(DESTDIR) ]; then \
		$(ECHO) "$(MKDIR) $(DESTDIR)"; \
		$(MKDIR) $(DESTDIR); \
		$(ECHO) "$(CHMOD) 755 $(DESTDIR)"; \
		$(CHMOD) 755 $(DESTDIR); \
		$(ECHO) "$(CHOWN) root $(DESTDIR)"; \
		$(CHOWN) root $(DESTDIR); \
		$(ECHO) "$(CHGRP) system $(DESTDIR)"; \
		$(CHGRP) system $(DESTDIR); \
	else \
		true; \
	fi
	@for i in $(FILES) $(EFILES) ; do \
		$(ECHO) "$(INSTALL) -c ../$$i $(DESTDIR)/$$i"; \
		$(INSTALL) -c ../$$i $(DESTDIR)/$$i; \
	done
	@for i in $(SFILES) ; do \
		$(ECHO) "$(INSTALL) -c ../$$i $(DESTDIR)/s.$$i"; \
		$(INSTALL) -c ../$$i $(DESTDIR)/s.$$i; \
	done

include $(GMAKERULES)
