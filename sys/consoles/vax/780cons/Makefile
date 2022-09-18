#  @(#)Makefile	4.1	ULTRIX	7/2/90
#									*
#			Copyright (c) 1984 by				*
#		Digital Equipment Corporation, Maynard, MA		*
#			All rights reserved.				*
#									*
#   This software is furnished under a license and may be used and	*
#   copied  only  in accordance with the terms of such license and	*
#   with the  inclusion  of  the  above  copyright  notice.   This	*
#   software  or  any  other copies thereof may not be provided or	*
#   otherwise made available to any other person.  No title to and	*
#   ownership of the software is hereby transferred.			*
#									*
#   This software is  derived  from  software  received  from  the	*
#   University    of   California,   Berkeley,   and   from   Bell	*
#   Laboratories.  Use, duplication, or disclosure is  subject  to	*
#   restrictions  under  license  agreements  with  University  of	*
#   California and with AT&T.						*
#									*
#   The information in this software is subject to change  without	*
#   notice  and should not be construed as a commitment by Digital	*
#   Equipment Corporation.						*
#									*
#   Digital assumes no responsibility for the use  or  reliability	*
#   of its software on equipment which is not supplied by Digital.	*
#									*
#***********************************************************************

#
# Make a 780/785 rx01 distribution (install) or console (update) floppy
#
# Modification History:
#
# tresvik - 13-apr-87
#	Yanked support update support and wired install to ../Makefile
# 

DESTROOT=
STDBOO= cira.cmd mbahp.cmd ubara.cmd cnsl.cmd
DIST=	${STDBOO} vmb.exe ci780.bin

all:

floppy:	${STDBOO} /usr/mdec/vmb.exe /usr/mdec/ci780.bin 
	cp /dev/null floppy
	/etc/arff ivf floppy 494,1,ULTRIX-32,Distribution,
	cp cnsl.cmd defboo.cmd
	cp /usr/mdec/vmb.exe vmb.exe
	cp /usr/mdec/ci780.bin ci780.bin
	/etc/arff rvf floppy defboo.cmd ${DIST}
	rm -rf vmb.exe defboo.cmd ci780.bin

install: floppy
	-for i in ${STDBOO} floppy ;do \
		install -c -m 644 $$i ${DESTROOT}/usr/sys/780cons/$$i; \
	done

clean:
	rm -f floppy vmb.exe ci780.bin

clobber:
	for i in `echo [a-z]*`; do \
		if test -f $$i; \
		then \
		rm -f $$i; \
		fi; \
	done
	rm -f Makefile

sccsget:
	sccs get ${DATE} -s SCCS

sccsinfo:
	sccs info
