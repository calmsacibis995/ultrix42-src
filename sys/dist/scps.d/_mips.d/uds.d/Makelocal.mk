#	Makelocal.mk
#		sys/dist/scps.d/_mips.d/uds.d Makefile
#
#	000	02-mar-1989	ccb
#	New.
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)


TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

SCPS= UDSRIS030.scp UDSVAXUPGRD030.scp

install:
	@for i in $(SCPS); \
	do \
		echo "$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i; \
	done
	
include $(GMAKERULES)
