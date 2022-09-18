#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib/learn/log $(DESTROOT)/usr/lib/learn/bin

FILES=	Linfo Xinfo

SUBDIRS=C editor eqn files macros morefiles vi

install:
	@for i in $(FILES); do \
		$(ECHO) "$(INSTALL) -c -m 644 ../$$i $(DESTROOT)/usr/lib/learn/$$i"; \
		$(INSTALL) -c -m 644 ../$$i $(DESTROOT)/usr/lib/learn/$$i; \
	done

include $(GMAKERULES)
