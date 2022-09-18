#	Makelocal.mk
#		sys/dist/scps.d/_mips.d/ Makefile
#
#	000	02-mar-1989	ccb
#	New.
#
#  @(#)Makelocal.mk	4.3  ULTRIX  8/2/90

include $(GMAKEVARS)


TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

#SCPS=  
#
#install:
#	@for i in $(SCPS); \
#	do \
#		echo "$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i"; \
#		$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i; \
#	done
install:
	@echo "Nothing to install"
	
include $(GMAKERULES)
