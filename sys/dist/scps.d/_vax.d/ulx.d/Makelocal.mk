#	Makelocal.mk
#		sys/dist Makefile
#
#	000	02-mar-1989	ccb
#	New.
#
#  @(#)Makelocal.mk	4.3  ULTRIX  8/2/90

include $(GMAKEVARS)

TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

#SCPLIST= 
#
#install:
#	@for i in $(SCPLIST); \
#	do \
#		echo "$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i"; \
#		$(INSTALL) -c -m 744 ../$$i $(TODIR)/$$i; \
#	done

install:
	@echo "Nothing to install."

include $(GMAKERULES)
