# @(#)Makelocal.mk	4.2      ULTRIX 	10/16/90

# Makefile for lpr.d/xlators.d
#
#
# AUTHOR:	Adrian Thoms
# DATE:		10th May 1989
#
# Modification History:
#
# 02-Oct-90 - Adrian Thoms (thoms@wessex)
#	Pass the prologue resource name into xlator_call via cpp
#	xlator_call is now a ksh script.

include $(GMAKEVARS)

include ../Makelocal_$(MACHINE).mk

XLDIR=usr/lib/lpdfilters

DESTLIST= $(DESTROOT)/$(XLDIR)

# This needs to match up with the version of the ansi prologue
# Currently this is to be found in xlators.d/ansi_ps.d/preamble.ps
# Note that the white space is crucial
#
ANSI_PROLOGUE = prologue lps_ansi_prologue V3.1-57

all:	xlator_call

xlator_call:	xlator_call.generic
	@$(ECHO) '/^#/s/.*//'>sed.tmp
	@$(ECHO) '/^	#/s/	#/#/' >> sed.tmp
	@$(ECHO) '#!/usr/bin/ksh' > $@
	$(CPP) -DANSI_PROLOGUE="\"${ANSI_PROLOGUE}\"" ../xlator_call.generic | sed -f sed.tmp  >> $@
	@$(RM) sed.tmp

install:
	install -c -m 755 xlator_call $(DESTROOT)/$(XLDIR)/xlator_call
	install -c -m 755 ../pr_call $(DESTROOT)/$(XLDIR)/pr_call


include $(GMAKERULES)
