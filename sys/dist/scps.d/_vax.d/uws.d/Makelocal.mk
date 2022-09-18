#	Makelocal.mk
#		sys/dist Makefile
#
#	000	02-mar-1989	ccb
#	New.
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

SCPLIST= UWSDECW021.scp UWSFONT021.scp UWSFONT15021.scp UWSMAN021.scp \
	UWSX11021.scp UWSXCOMP021.scp UWSXDEV021.scp

install:
	@for i in $(SCPLIST); \
	do \
		echo "$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i; \
	done

include $(GMAKERULES)
