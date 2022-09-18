#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	at

OBJS=	at.o

at:	at.o
at.o:	at.c

install:
	-@if [ ! -d $(DESTROOT)/usr/var/spool/at ]; then \
		$(ECHO) "$(MKDIR) $(DESTROOT)/usr/var/spool/at"; \
		$(MKDIR) $(DESTROOT)/usr/var/spool/at; \
		$(ECHO) "$(CHMOD) 755 $(DESTROOT)/usr/var/spool/at"; \
		$(CHMOD) 755 $(DESTROOT)/usr/var/spool/at; \
		$(ECHO) "$(CHOWN) root $(DESTROOT)/usr/var/spool/at"; \
		$(CHOWN) root $(DESTROOT)/usr/var/spool/at; \
		$(ECHO) "$(CHGRP) system $(DESTROOT)/usr/var/spool/at"; \
		$(CHGRP) system $(DESTROOT)/usr/var/spool/at; \
	else \
		true; \
	fi
	-@if [ ! -d $(DESTROOT)/usr/var/spool/at/past ]; then \
		$(ECHO) "$(MKDIR) $(DESTROOT)/usr/var/spool/at/past"; \
		$(MKDIR) $(DESTROOT)/usr/var/spool/at/past; \
		$(ECHO) "$(CHMOD) 755 $(DESTROOT)/usr/var/spool/at/past"; \
		$(CHMOD) 755 $(DESTROOT)/usr/var/spool/at/past; \
		$(ECHO) "$(CHOWN) root $(DESTROOT)/usr/var/spool/at/past"; \
		$(CHOWN) root $(DESTROOT)/usr/var/spool/at/past; \
		$(ECHO) "$(CHGRP) system $(DESTROOT)/usr/var/spool/at/past"; \
		$(CHGRP) system $(DESTROOT)/usr/var/spool/at/past; \
	else \
		true; \
	fi
	$(INSTALL) -c -s -m 4755 at $(DESTROOT)/usr/bin/at
	$(RM) $(DESTROOT)/usr/var/spool/at/at.deny
	$(TOUCH) $(DESTROOT)/usr/var/spool/at/at.deny
	-$(CHMOD) 644 $(DESTROOT)/usr/var/spool/at/at.deny
	-$(CHOWN) root $(DESTROOT)/usr/var/spool/at/at.deny
	$(RM) $(DESTROOT)/usr/lib/cron
	$(LN) -s /usr/var/spool/at $(DESTROOT)/usr/lib/cron

include $(GMAKERULES)
