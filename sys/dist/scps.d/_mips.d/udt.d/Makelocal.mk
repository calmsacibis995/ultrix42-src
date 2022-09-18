#	Makelocal.mk
#		sys/dist/scps.d/_mips.d/ Makefile
#
#	000	02-mar-1989	ccb
#	New.
#
#	001	20-Feb-1991	overman
#	added subsets to create COMU4BACKEND205 workaround for V4.2
#
#
#  @(#)Makelocal.mk	4.8	ULTRIX	4/4/91

include $(GMAKEVARS)

SUBDIRS= subsets.d

TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

SCPS= UDTDL420.scp UDTFIS420.scp UDTBASE421.scp

install:
	@for i in $(SCPS); \
	do \
		echo "$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i; \
	done
	
include $(GMAKERULES)
