#	Makelocal.mk
#		sys/dist Makefile
#
#	000	02-mar-1989	ccb
#	New.
#
#	001	10-Dec-90	overman
#	rev'ed the version number on the scp's
#
#  @(#)Makelocal.mk	4.3  ULTRIX  12/20/90

include $(GMAKEVARS)

TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

SCPLIST= ULCCRYPT420.scp ULCPGMR420.scp

install:
	@for i in $(SCPLIST); \
	do \
		echo "$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i; \
	done

include $(GMAKERULES)
