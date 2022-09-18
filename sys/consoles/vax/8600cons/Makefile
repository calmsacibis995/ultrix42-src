#	5/21/86	ULTRIX	@(#)Makefile	1.2
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
#
# Modification History:
#
# tresvik - 13-apr-87
#	Wired install to ../Makefile
# 

DESTROOT=
STDBOO=cira.com ubara.com mbahp.com cnsl.com

all:

install:
	-for i in ${STDBOO} ;do \
		install -c -m 644 $$i ${DESTROOT}/usr/sys/8600cons/$$i; \
	done

clean:
	
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

