# @(#)Makelocal.mk	4.2      ULTRIX 	10/16/90

# Makefile for print.d
#
#
# AUTHOR:	Adrian Thoms
# DATE:		28th February 1989
#
# Modification History
#
# 24-sep-90	Adrian Thoms (thoms@wessex)
#	Added libfilter.d
#
# 08-nov-89	thoms
#	Added xopen.d for compatibility scripts

include $(GMAKEVARS)

SUBDIRS=h.d lib.d utils.d xlators.d dcl.d lprsetup.d xopen.d \
	libfilter.d filters.d lpsfilters.d 

sccsget:	sccsget_xlators_local

sccsget_xlators_local:
	( cd ../xlators.d; $(SCCS) get Makelocal_$(MACHINE).mk )

include $(GMAKERULES)
