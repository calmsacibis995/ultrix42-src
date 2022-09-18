#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
# Make template for Makelocal.mk
# Should be used to make sources within a directory and subdirectories

# First thing is to include the general variables for the machine
# type.
include $(GMAKEVARS)

install:

# All vars are set above. 
# Include rules file that will perform operations based on those vars.
include $(GMAKERULES)
