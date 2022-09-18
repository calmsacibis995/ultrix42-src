#  @(#)Makeold.mk	2.1  ULTRIX  4/24/89

#
# allows .DEFAULT: to triggers instead of .?.? rules
#
.SUFFIXES:
.SUFFIXES: .z

CD=cd
MAKE=s5make

#
# Generic variable and macro definitions
#
include $(SRCROOT)/genmake.d/Makevars_$(MACHINE).mk

#
# convert cleanall and clean$(MACHINE) back to clean
#
cleanall cleanmips cleanvax: clean

#
# convert sccsgetmips back to sccsget
#
sccsgetall sccsgetmips sccsgetvax: sccsget

#
# anything else just passes through
#
.DEFAULT:
	$(CD) .. ;\
	$(MAKE) -$(MAKEFLAGS) $(MAKEDEFS) DESTROOT=$(DESTROOT) $@
