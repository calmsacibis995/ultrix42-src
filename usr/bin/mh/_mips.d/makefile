#  @(#)makefile	4.2  ULTRIX  3/18/91
#
# Makefile to drive the real MH makefiles
# and mask their non-standardness from the Ultrix-32 build
# procedures.
#
# This makefile is written to emulate all of the necessary
# Ultrix build procedure rules without teaching all of the subordinate
# makefiles how this is done.
#
#	@(#)Make.build.only	1.9	Ultrix	6/11/86

# Makefile used to manipulate sources within the directory and
# containing no subdirectories.
#

# All makefiles should contain keywords.  In order to get the keywords
# to stay unexpanded, I had to add the backslashes.  So when putting the
# keywords into a makefile, please use the following line without the
# backslashes.

#	\%W\%	Ultrix	\%G\%

OBJS =

CHOWN= /etc/chown

DOCDIRS = $(DESTROOT)/usr/man $(DESTROOT)/usr/man/manl

DIRS=	$(DESTROOT)/usr $(DESTROOT)/usr/bin $(DESTROOT)/usr/bin/mh \
	$(DESTROOT)/usr/lib $(DESTROOT)/usr/lib/mh \
	$(DESTROOT)/etc $(DESTROOT)/etc/popd $(DOCDIRS)

BBDIRS=	$(DESTROOT)/usr/spool $(DESTROOT)/usr/spool/bboards \
	$(DESTROOT)/usr/spool/bboards/etc

INSTFLAGS =
CFLAGS =

LINT=
LINTFLAGS=

SCRIPTS =

STD =

NSTD =

#
# Any local variables that are specific to the Makefile should go after
# the above variables.  For example, you may want to use different
# variables for specifying directories where things get installed.
# Also, here is where any variables dealing with the tools rules 
# should live.

#
# For the first rule within the makefile, you must have the "all"
# rule.  This builds all the sources.
#
# Well, the ULTRIX build breaks the above rule.  The "clean" rule is
# actually run first.  So, because of that we build the makefiles
# both here in "all" and in "clean" -- just to make sure we have
# them!
#

all: ${STD} ${NSTD}
	-chmod 755 ./conf/mhdoc
	-(cd conf; $(MAKE) ; ./mhconfig MH)
	-$(MAKE) -f Makefile

#${STD}:
#	${CC} ${CFLAGS}
#
#${NSTD}:
#	${CC} ${CFLAGS}
#
#
# Install will place the objects and executables onto the output pack.
# If the STD files are to live in the "etc" directory the install rule
# could look like this - 
#	for i in ${STD}; do \
#	(install ${INSTFLAGS} -c $$i ${DESTROOT}/etc/$$i); done
# If you also have NSTD files they should be installed in a similar
# fashion.  
#

install: ${DIRS} ${BBDIRS}
	$(MAKE) -f Makefile DESTDIR="${DESTROOT}" inst-all

${DIRS} ${BBDIRS}:
	mkdir $@
	chmod 755 $@

#
# Removes core and a.out files, sources, and objects from within CWD.
#
# Since in the ULTRIX build we do the "clean" rule before we do the
# "all" rule, and we need to have conf/mhconfig generate "Makefile", 
# we must build the makefiles before doing the clean.
#

clean:
	-chmod 755 ./conf/mhdoc
	-(cd conf; $(MAKE) ; ./mhconfig MH)
	-$(MAKE) -f Makefile clean

#
# Rule to remove all files within CWD and leave only the SCCS directory 
# (this assumes that all files needed within the directory live in
# sccs).  
#

clobber:
	find . -type f \! -user sccs -exec rm -f {} \;


#
# Rule to get all sources out of sccs after a clobber was done or 
# just to make sure that the lastest of all files are within directory.
#

sccsget:
	@find . -type d -name "SCCS" -print | \
	sed -e 's=/SCCS$$==' -e 's=.*=(cd &; sccs get SCCS)=' | sh
	
#
# Rule to see if anything is checked out of sccs for edit.
#

sccsinfo:
	find . -type d -print | grep -v SCCS | \
			sed 's=.*=(cd &; ${SCCS} info)=' | sh

#
# This rule when run produces a tags file which gives the locations
# of specified objects in a group of files
#

tags:
	find . -type d -print | grep -v SCCS | \
			sed 's=.*=(cd &; ctags *.[ch])=' | sh

#
# This rule is run to detect features of the C program files which
# are likely to be bugs, nonportable or wasteful.
#

lint:
	$(MAKE) -f Makefile lint

#
# Any rules specific to this Makefile.  At times, the above standard
# rules may not be enough to do the job.  Additional rules should
# live here and not be mixed in with the standard rules.  This 
# may be any tools rules, or if the "all" rule has only a list
# of dependencies and there are several rules to actually build
# within the directory they should also live here.
#

# 
# The depend rule should be here at the bottom of the file.
# The depend rule creates dependency lines at the end of the 
# makefile when it is ran.  It is a script that automates the
# building of dependencies. The following is an example of 
# a depend rule.
#

depend:
	$(MAKE) -f Makefile depend

