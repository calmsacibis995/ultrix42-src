#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/man

install:
	cd $(DESTROOT)/usr/man; \
		tar cf - -C $(SRCROOT)/usr/man/_$(MACHINE).d . | tar xf -; \
		find . -type d -print | xargs -n200 chmod 755; \
		find . -type f -print | xargs -n200 chmod 444; \
		find . -print | xargs -n200 chown root; \
		find . -print | xargs -n200 chgrp system

include $(GMAKERULES)
