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
# Make a VAX 11/750 TU58 distribution BOOT 1/1 kit image
#
# Modification History:
#

DESTROOT=
STDBOO= cira.cmd cnsl.cmd

all:

750cons: ${STDBOO} /usr/mdec/vmb.exe /usr/mdec/ci780.bin /usr/mdec/pcs750.bin
	dd if=750console of=bblock count=1
	/etc/arff imf cons 512,1,ULTRIX-32,Console, 
	dd if=cons of=cons1 skip=1
	cat bblock cons1 > 750cons
	rm -f cons cons1 bblock
	rm -rf tmp
	mkdir tmp; \
		cd tmp;\
		arff xmf ../750console; \
		rm -f *.cmd *xdt *gen ; \
		cp ../cnsl.cmd defboo.cmd; \
		cp ../cnsl.cmd .; \
		cp ../cira.cmd .; \
		cp /usr/mdec/vmb.exe vmb.exe; \
		cp /usr/mdec/ci780.bin ci780.bin; \
		cp /usr/mdec/pcs750.bin pcs750.bin; \
		/etc/arff rmf ../750cons *
	rm -rf tmp
	- set xx `arff tmvf 750cons | grep BOOT58.EXE` ;\
	while [ "$$2" != "BOOT58.EXE" ]; do \
		shift;\
	done; \
	SIZE=$$3; LBN=$$5; \
	echo "20?W 0t$$SIZE" >> /tmp/750cons.adb ;\
	echo "6?w 0t$$LBN" >> /tmp/750cons.adb
	adb -w 750cons < /tmp/750cons.adb 
	rm -f /tmp/750cons.adb

install: 750cons
	-for i in ${STDBOO} 750cons ;do \
		install -c -m 644 $$i ${DESTROOT}/usr/sys/750cons/$$i; \
	done

clean:
	rm -f 750cons vmb.exe ci780.bin pcs750.bin

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
