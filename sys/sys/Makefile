#
#	@(#)Makefile	1.3 (ULTRIX)2/28/89
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
TARDIRS=vax mips

all:

clean:

clobber: clean
	@cd ${MACHINE};sccs clean;

sccsinfo:
	@cd ${MACHINE};echo -n "${MACHINE}:";sccs info

sccsget:
	@-cd ${MACHINE};echo "${MACHINE}:";sccs get -s SCCS

install:

