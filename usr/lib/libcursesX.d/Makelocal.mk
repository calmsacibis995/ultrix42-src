# @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90
#
#	Curses Library High Level Makefile.
#
include $(GMAKEVARS)
SUBDIRS= local screen terminfo
TOOLSDIRS= screen terminfo

pretools tools1 tools2 install:	$(TOOLSDIRS)

include $(GMAKERULES)
