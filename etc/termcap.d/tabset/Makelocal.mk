#  @(#)Makelocal.mk	4.2  ULTRIX  11/15/90

include $(GMAKEVARS)

DESTDIR=$(DESTROOT)/usr/lib/tabset

DESTLIST=$(DESTDIR)

FILES=3101 aa beehive diablo std stdcrt teleray vt100 vt300 xerox1720

install:
	@for i in $(FILES); do \
		$(ECHO) "$(INSTALL) -c ../$$i $(DESTDIR)/$$i"; \
		$(INSTALL) -c ../$$i $(DESTDIR)/$$i; \
	done

include $(GMAKERULES)
