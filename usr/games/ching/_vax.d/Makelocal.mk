#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

DESTLIB=${DESTROOT}/usr/games/lib/ching.d

install:
	$(INSTALL) -c ../ching.sh ${DESTROOT}/usr/games/ching
	-@if [ ! -d $(DESTLIB) ]; then \
		$(ECHO) "$(MKDIR) $(DESTLIB)"; \
		$(MKDIR) $(DESTLIB); \
		$(ECHO) "$(CHMOD) 755 $(DESTLIB)"; \
		$(CHMOD) 755 $(DESTLIB); \
		$(ECHO) "$(CHOWN) root $(DESTLIB)"; \
		$(CHOWN) root $(DESTLIB); \
		$(ECHO) "$(CHGRP) system $(DESTLIB)"; \
		$(CHGRP) system $(DESTLIB); \
	else \
		true; \
	fi
	$(INSTALL) -c ../cno $(DESTLIB)/cno
	$(INSTALL) -c ../phx $(DESTLIB)/phx
	$(INSTALL) -c ../macros $(DESTLIB)/macros
	$(INSTALL) -c ../hexagrams $(DESTLIB)/hexagrams

include $(GMAKERULES)
