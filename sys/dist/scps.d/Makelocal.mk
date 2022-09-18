#	Makelocal.mk
#		sys/dist Makefile
#
#	000	02-mar-1989	ccb
#	New.
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

SUBDIRS= _$(MACHINE).d

include $(GMAKERULES)
