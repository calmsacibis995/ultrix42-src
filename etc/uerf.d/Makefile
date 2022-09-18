#  @(#)Makefile	1.2  ULTRIX  3/7/89

#
# SRCROOT should be set in the Shell environment
#
#SRCROOT= $(SRCROOT)

#
# MACHINE is set by make
#	currently either vax or mips
#
#MACHINE = $(MACHINE)

#
# allows .DEFAULT: to triggers instead of .?.? rules
#
.SUFFIXES:
.SUFFIXES: .z

#
# Generic variable and macro definitions
#
include $(SRCROOT)/genmake.d/Makevars_$(MACHINE).mk

MAKERULE=$@
#
# "make all" is the default
#
all:
	($(CD) _$(MACHINE).b;\
	$(MAKECMD) all)

.DEFAULT:
	($(CD) _$(MACHINE).b;\
	$(MAKECMD) $@)
