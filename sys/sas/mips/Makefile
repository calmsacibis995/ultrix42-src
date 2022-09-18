#
#  @(#)Makefile	4.4	(ULTRIX)	10/15/90
#
#			Copyright (c) 1985 by
#		Digital Equipment Corporation, Maynard, MA
#			All rights reserved.
#
#   This software is furnished under a license and may be used and
#   copied  only  in accordance with the terms of such license and
#   with the  inclusion  of  the  above  copyright  notice.   This
#   software  or  any  other copies thereof may not be provided or
#   otherwise made available to any other person.  No title to and
#   ownership of the software is hereby transferred.
#
#   This software is  derived  from  software  received  from  the
#   University    of   California,   Berkeley,   and   from   Bell
#   Laboratories.  Use, duplication, or disclosure is  subject  to
#   restrictions  under  license  agreements  with  University  of
#   California and with AT&T.
#
#   The information in this software is subject to change  without
#   notice  and should not be construed as a commitment by Digital
#   Equipment Corporation.
#
#   Digital assumes no responsibility for the use  or  reliability
#   of its software on equipment which is not supplied by Digital.
#
# Revision History:
#
# Oct 09, 1990 - Joe Szczypek
#	Modified build of libasm so that it can be build in a small form
# 	form bootblks, and a bigger faster version for other.
#
DESTROOT=
INCLUDE=-I..
CFLAGS=-O ${INCLUDE} -EL
MIPS_RELOC=80700000
RELEASE=V4.2
#
# General maintenance rules
#
BOOTS = ultrixboot bootblks
LOADERS = netload
ALL = ${BOOTS} ${LOADERS}
all: ${ALL}

insthdrs:

install: all
	-if [ ! -d ${DESTROOT}/usr/mdec ] ; then \
		mkdir ${DESTROOT}/usr/mdec; \
		chmod 755 ${DESTROOT}/usr/mdec; \
		/etc/chown root ${DESTROOT}/usr/mdec; \
		chgrp system ${DESTROOT}/usr/mdec; \
		else true; \
	fi
	install -s -m 644 -c netload ${DESTROOT}/usr/mdec/netload
	install -s -m 644 -c ultrixboot ${DESTROOT}/ultrixboot
	install -m 644 -c bootblks ${DESTROOT}/usr/mdec/bootblks

clean:
	rm -f *.out core ${BOOTS} ${LOADERS} *.o errs \
		makefile.bak

clobber:
# save the bootlblk binary piece across a clobber
	mv bootblk BOOTBLK
# touch a junk file here to satisfy the shell expansion in case
# of consecutive clobbers
	touch junk
	for i in `echo [a-z]*`; do \
		if test -f $$i; \
		then \
		rm -f $$i; \
		fi; \
	done
	mv BOOTBLK bootblk
	rm -f Makefile

sccsget:
	sccs get ${DATE} -s SCCS

sccsinfo:
	sccs info

#
# Specific rules
#

version.c: FRC
	echo -n "char *version=\"" > version.c; \
	echo -n "$(RELEASE) " `date` \
		   >> version.c; \
	echo "\";" >> version.c

version.o: version.c
	-rm -f version.o
	cc $(CFLAGS) -c version.c

sys.o sys.SECONDARY.o: ../sys.c
	cc -c ${CFLAGS} -DSECONDARY ../sys.c
	mv sys.o sys.SECONDARY.o
	cc -c ${CFLAGS} ../sys.c

loadimage.o loadimage.SECONDARY.o: loadimage.c
	cc -c ${CFLAGS} -DSECONDARY loadimage.c
	mv loadimage.o loadimage.SECONDARY.o
	cc -c ${CFLAGS} loadimage.c

ultrixboot.o: ../ultrixboot.c
	cc ${CFLAGS} -c ../ultrixboot.c

netload.o: ../netload.c
	cc ${CFLAGS} -c ../netload.c

mipsboot.o: mipsboot.c
	cc ${CFLAGS} -c mipsboot.c

io.o: ../io.c
	cc ${CFLAGS} -c ../io.c

decompress.o: ../decompress.c
	cc ${CFLAGS} -c ../decompress.c

bootblk: FRC

FRC:

#
# VAX boot image which resides in LBNs 0-15
#
bootblks: csu.mips.o prom_entry.o libasm.o mipsboot.o \
		io.o sys.o loadimage.o
	ld -T 80600000 -N csu.mips.o prom_entry.o libasm.o \
		mipsboot.o io.o sys.o loadimage.o -lc
	cp a.out A.OUT
	strip A.OUT; dd if=A.OUT of=b.out ibs=288 skip=1 2> /dev/null
	rm -f A.OUT
	cat bootblk b.out > bootblks
	ls -l bootblks

#
# ultrixboot rules
#
ultrixboot: csu.mips.o prom_entry.o libasm.SECONDARY.o ultrixboot.o \
		sys.SECONDARY.o io.o loadimage.SECONDARY.o
	make version.o; ld -N -T ${MIPS_RELOC} -o ultrixboot \
		csu.mips.o libasm.SECONDARY.o \
		prom_entry.o ultrixboot.o sys.SECONDARY.o io.o \
		loadimage.SECONDARY.o version.o -lc; rm -f version.o


#
# netload rules
#
netload: csu.mips.o prom_entry.o \
		libasm.SECONDARY.o netload.o
	make version.o; ld -N -T ${MIPS_RELOC} -o netload \
		csu.mips.o prom_entry.o \
		libasm.SECONDARY.o netload.o version.o; rm -f version.o

csu.mips.o: csu.mips.s
	as -EL ${INCLUDE} -DLOCORE -DSTANDALONE -o csu.mips.o csu.mips.s

prom_entry.o: prom_entry.s
	as -EL ${INCLUDE} -DLOCORE -DMIPS -DSTANDALONE -o prom_entry.o prom_entry.s

libasm.o libasm.SECONDARY.o: libasm.s
	as -EL ${INCLUDE} -DLOCORE -DMIPS -DSTANDALONE -DSECONDARY -o libasm.o libasm.s
	mv libasm.o libasm.SECONDARY.o
	as -EL ${INCLUDE} -DLOCORE -DMIPS -DSTANDALONE -o libasm.o libasm.s

depend:
	grep '^#include' ${SRCS} | grep -v '<' | \
	sed -e 's/:[^"]*"\([^"]*\)".*/: \1/' \
	    -e 's/\.c/.o/' > xtmp 
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep <xtmp
	rm -f xtmp
	echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile makefile.bak
	ed - Makefile < eddep
	rm eddep makedep

# DO NOT DELETE THIS LINE -- make depend needs it
