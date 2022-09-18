#  @(#)Makefile	4.1	(ULTRIX)	7/2/90
#									*
#			Copyright (c) 1988 by				*
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
#   The information in this software is subject to change  without	*
#   notice  and should not be construed as a commitment by Digital	*
#   Equipment Corporation.						*
#									*
#   Digital assumes no responsibility for the use  or  reliability	*
#   of its software on equipment which is not supplied by Digital.	*
#									*
#***********************************************************************

#
# Make a VAX 8200 RX50 distribution BOOT 1/1 kit image
#
# Modification History:
#

DESTROOT=
STDBOO= cira.cmd cnsl.cmd

all:

8200cons: ${STDBOO} /usr/mdec/vmb.exe /usr/mdec/ci780.bin /usr/mdec/cibca.bin
	dd if=8200console of=bblock count=1
	/etc/arff imf cons 800,1,ULTRIX-32,Console, 
	dd if=cons of=cons1 skip=1
	cat bblock cons1 > 8200cons
	rm -f cons cons1 bblock
	rm -rf tmp
	mkdir tmp; \
		cd tmp;\
		arff xmf ../8200console; \
		rm -f *.cmd; \
		cp ../cnsl.cmd defboo.cmd; \
		cp ../cnsl.cmd .; \
		cp ../cira.cmd .; \
		cp /usr/mdec/vmb.exe .; \
		cp /usr/mdec/ci780.bin .; \
		cp /usr/mdec/cibca.bin .; \
		/etc/arff rmf ../8200cons *
	rm -rf tmp
	- set xx `arff tmvf 8200cons | grep BOOT58.EXE` ;\
	while [ "$$2" != "BOOT58.EXE" ]; do \
		shift;\
	done; \
	SIZE=$$3; LBN=$$5; \
	echo "20?W 0t$$SIZE" >> /tmp/8200cons.adb ;\
	echo "6?w 0t$$LBN" >> /tmp/8200cons.adb
	adb -w 8200cons < /tmp/8200cons.adb 
	rm -f /tmp/8200cons.adb

install: 8200cons
	-for i in ${STDBOO} 8200cons ;do \
		install -c -m 644 $$i ${DESTROOT}/usr/sys/8200cons/$$i; \
	done

clean:
	rm -f 8200cons vmb.exe ci780.bin cibca.bin

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
