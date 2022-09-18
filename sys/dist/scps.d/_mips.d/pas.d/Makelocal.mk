#	Makelocal.mk
#		sys/dist/scps.d/_mips.d/f77.d Makefile
#
#	000	02-mar-1989	ccb
#	New.
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)


TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

SCPS= PASBASE010.scp

install:
	@for i in $(SCPS); \
	do \
		echo "$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i; \
	done
	
include $(GMAKERULES)
