#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
# Make template for Makelocal.mk
# Should be used to make sources within a directory and subdirectories

# First thing is to include the general variables for the machine
# type.
include $(GMAKEVARS)

# If the directory contains many sources to create a single a.out
# file list the a.out name in AOUT.  List the objects for the a.out
# in OBJS below.
# Example: In the /src/bin/diff area this will be defined like
#	AOUT=diff
AOUT=dc

# If the directory will be used to make an AOUT or ARFILE then list
# the objects for it here in OBJS.  You must explicity list
# dependencies for each object.
# Example: In the /src/bin/diff area this will be defined like
#	OBJS=diff.o diffdir.o diffreg.o
#	diff.o:diff.c diff.h
#	diffdir.o:diffdir.c diff.h
#	diffreg.o:diffreg.c diff.h
#
OBJS=dc.o
dc.o:dc.c dc.h

# If the directory contains c source files to compiled then
# list them in CFILES.  Otherwise leave it blank.
# This should include all cfiles used in defining any variable above.
# Example: For the /src/bin example above it would list
#	CFILES=
CFILES=dc.c
HFILES=dc.h

# Install has no generic rules.  You must handcraft the install
# rule?
install:
	install -c -s dc ${DESTROOT}/usr/bin/dc

# All vars are set above.  Include rules file that will perform operations
# based on those vars.
include $(GMAKERULES)

