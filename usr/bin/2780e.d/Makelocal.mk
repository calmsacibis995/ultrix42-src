#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
# Make template for Makelocal.mk
# Should be used to make sources within a directory and subdirectories

# First thing is to include the general variables for the machine
# type.
include $(GMAKEVARS)

# If the directory contains subdirectories that have their
# own make procedures list them in SUBDIRS.  They will be executed
# in the order that you list them.  This list of subdirectories
# is used by all the rules to step through the build area.
# Example: In the /src area this will be defined something like
#	SUBDIRS=bin etc usr   ...(and so on)
SUBDIRS=_$(MACHINE).d

# All vars are set above.
# Include rules file that will perform operations based on those vars.
include $(GMAKERULES)
