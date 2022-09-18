#	Makelocal.mk
#		sys/dist Makefile
#
#	001	17-jul-1990	mdf
#	Reved up for V4.1
#
#	000	02-mar-1989	ccb
#	New.
#
#  @(#)Makelocal.mk	4.5  ULTRIX  12/20/90

include $(GMAKEVARS)

TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

SCPLIST= ULTDL420.scp ULTBASE421.scp

install:
	@for i in $(SCPLIST); \
	do \
		echo "$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i; \
	done

include $(GMAKERULES)
