#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTDIR=$(DESTROOT)/usr/lib/me

FILES= acm.me chars.me deltext.me eqn.me float.me footnote.me index.me \
	local.me null.me refer.me revisions sh.me tbl.me thesis.me

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
	@for i in $(FILES); do \
		$(ECHO) "$(INSTALL) -c ../$$i $(DESTDIR)/$$i"; \
		$(INSTALL) -c ../$$i $(DESTDIR)/$$i; \
	done

include $(GMAKERULES)
