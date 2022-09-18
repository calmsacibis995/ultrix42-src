#	Makelocal.mk
#		sys/dist/scps.d/_mips.d/udc.d Makefile
#
#	000	02-mar-1989	ccb
#	New.
#
#	001	12-Dec-90	overman
#	reved up to V4.2 names
#
#  @(#)Makelocal.mk	4.3  ULTRIX  12/20/90

include $(GMAKEVARS)


TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

SCPS= UDCCRYPT420.scp UDCPGMR420.scp

install:
	@for i in $(SCPS); \
	do \
		echo "$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i; \
	done
	
include $(GMAKERULES)
