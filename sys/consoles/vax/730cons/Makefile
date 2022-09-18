#  @(#)Makefile	4.1	ULTRIX	7/2/90
#									*
#			Copyright (c) 1983 by				*
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
# Make a 730 tu58 cassette
#
# Modification History:
#
# tresvik - 13-apr-87
#	Yanked support update support and wired install to ../Makefile
# 
DESTROOT=
STDBOO=	ubara.cmd ubaidc.cmd cnsl.cmd

all:

cassette: ${STDBOO} /usr/mdec/vmb.exe
	cp /dev/null cassette
	cp cnsl.cmd defboo.cmd
	cp /usr/mdec/vmb.exe .
	/etc/arff imvf cassette 512,1,ULTRIX-32,Distribution,
	/etc/arff rmvf cassette defboo.cmd vmb.exe ${STDBOO}
	rm -f vmb.exe defboo.cmd

install: cassette
	-for i in ${STDBOO} cassette ;do \
		install -c -m 644 $$i ${DESTROOT}/usr/sys/730cons/$$i; \
	done

clean:
	rm -f vmb.exe cassette

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
