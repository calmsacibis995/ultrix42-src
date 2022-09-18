#  @(#)Makevars_mips.mk	1.1  ULTRIX  2/24/89

#
# generic variable and macro definitions for mips
#

#
# MACHINE should be set by make
#
#MACHINE =	mips

#
# SRCROOT should be in the Shell environment
#
#SRCROOT =	/usr/projects/pu

#
# DESTROOT is the root directory of where to install
#
DESTROOT =	$(SRCROOT)/Ultrix.mips

#
# DESTLIST is a list directories to install
#	*** must be of the form $(SRCROOT)/... ***
DESTLIST =	nodestlist

#
# SUBDIRS is a list of directories to continue the make traversal
#
SUBDIRS =	nosubdirs

#
# ar - archive
# ranlib
#
ARFLAGS =	crv
AR =		ar
RANLIB =	ranlib

ARFILE =	noarfile
OBJS =

#
# as - assembler
#
ASFLAGS =
AS =		as

#
# cc - C compiler
#
CDEBUG =
CDEFINES =
CFLAGS =	-O
CINCLUDES =	-I. -I.. -I$(SRCROOT)/usr/include
CC =		cc
CCCMD =		$(CC) -c $(CDEBUG) $(CFLAGS) $(CDEFINES) $(CINCLUDES)

HFILES =
CFILES =

#
# f77 - FORTRAN, RATFOR, ...
#
FFLAGS =	-O
FC =		f77
EFLAGS =	-O
EC =		f77
RFLAGS =	-O
RC =		f77

#
# ld - link editor
#
LDFLAGS =
LD =		ld
LDCMD =		$(CC) $(LDFLAGS) -o $@

LOADLIBES =
OBJS =
AOUT =		noaout
AOUTS =		noaouts

#
# lex - lexical analyser
#
LFLAGS =
LEX =		lex

#
# make
#
VPATH =		..
MAKEDEFS =	DESTROOT=$(DESTROOT)
MAKEFLAGS =
MAKERULE =
MAKE =		s5make
MAKECMD =	$(MAKE) -$(MAKEFLAGS) $(MAKEDEFS) \
		GMAKEVARS=$(GMAKEVARS) MAKERULE=$(MAKERULE) \
		-f ../$(MAKELOCAL)

GMAKEVARS =	$(SRCROOT)/genmake.d/Makevars_$(MACHINE).mk
GMAKERULES =	$(SRCROOT)/genmake.d/Makerules.mk
INCLUDEVARS =	$(SRCROOT)/genmake.d/Makevars_$(MACHINE).mk
INCLUDERULES =	$(SRCROOT)/genmake.d/Makerules.mk
MAKELOCAL =	Makelocal.mk
MAKEFILE =	Makefile
MAKEFILES =	$(MAKELOCAL) Makefile

#
# miscellaneous commands
#
AWK =		awk
CD =		cd
CHGRP =		chgrp
CHMOD =		chmod
CHOWN =		/etc/chown
CPP =		/lib/cpp
DBGECHO =	:
ECHO =		echo
INSTALL =	install
LN =		ln
MKDIR =		mkdir
MV =		mv
RM =		rm -f
STRIP =		strip
TOUCH =		touch
XSTR =		xstr

#
# pc - Pascal
#
PFLAGS =	-O
PC =		pc

#
# sccs - source code control
#
SCCSCLEAN =	clean
SCCSCLEANCMD =	$(SCCS) $(SCCSCLEAN)
SCCSGET =	get SCCS
SCCSGETCMD =	$(SCCS) $(SCCSGET)
SCCS =		sccs


#
# the Shell
#
SHELL =		/usr/bin/sh5

#
# yacc
#
YFLAGS =
YACC =		yacc
