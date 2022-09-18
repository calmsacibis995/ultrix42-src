#	Makelocal.mk -
#		sys/dist/etc.d/rc.d Makefile
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
#	000	02-mar-1989	ccb
#	New.
#

include $(GMAKEVARS)

TODIR= $(DESTROOT)/etc
DESTLIST= $(TODIR)

RCFILES= rc rc.local

install:
	@for i in $(RCFILES); \
	do \
		echo "$(INSTALL) -c -m 745 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 745 ../$$i $(TODIR)/$$i; \
	done

include $(GMAKERULES)
