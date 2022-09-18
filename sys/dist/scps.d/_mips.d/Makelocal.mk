#	Makelocal.mk
#		sys/dist/scps.d/_mips.d/ Makefile
#
#	000	02-mar-1989	ccb
#	New.
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

SUBDIRS= f77.d pas.d udc.d uds.d udt.d udw.d udx.d

include $(GMAKERULES)
