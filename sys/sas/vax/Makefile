#  @(#)Makefile	4.2	(ULTRIX)	9/4/90
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
#

DESTROOT=
INCLUDE=-I.. -I../../VAX/BINARY
CFLAGS=-O ${INCLUDE}
# Relocation factor boot path and loader path at 2 1/2 Meg 
VAX_RELOC=280000
RELEASE=4.2

BOOTS = ultrixboot bootblks
LOADERS = ultrixload netload 

SRCS=ultrixboot.c sys.c netload.c ultrixload.c decompress.c 
#
# General maintenance rules
#
ALL = ${BOOTS} ${LOADERS}

all: ${ALL}

SRCS=	bootblk.c common.c qdcons.c qdio.c cons60.c cons640.c cons650.c \
	qvcons.c vaxstart.c ultrixstart.c \
	pcsload.c qio.c geterr.c emulate.c \
	prf.c get.c loadimage.c

insthdrs:
	-if [ ! -d ${DESTROOT}/usr/sys/sas/vax ]; \
	then \
		mkdir ${DESTROOT}/usr/sys/sas/vax; \
		/etc/chown root ${DESTROOT}/usr/sys/sas/vax; \
	else \
		true; \
	fi
	install -m 644 -c vmb.h ${DESTROOT}/usr/sys/sas/vax/vmb.h

install: all
	-if [ ! -d ${DESTROOT}/usr/mdec ]; \
	then \
		mkdir ${DESTROOT}/usr/mdec; \
		/etc/chown root ${DESTROOT}/usr/mdec; \
	else \
		true; \
	fi
	install -m 644 -c ultrixload ${DESTROOT}/usr/mdec/ultrixload
	install -s -m 644 -c netload ${DESTROOT}/usr/mdec/netload
	install -s -m 644 -c ultrixboot ${DESTROOT}/ultrixboot
	install -m 644 -c bootblks ${DESTROOT}/usr/mdec/bootblks
	-if [ ! -d ${DESTROOT}/usr/sys/sas/vax ]; \
	then \
		mkdir ${DESTROOT}/usr/sys/sas/vax; \
		/etc/chown root ${DESTROOT}/usr/sys/sas/vax; \
	else \
		true; \
	fi
	install -m 644 -c vmb.h ${DESTROOT}/usr/sys/sas/vax/vmb.h


clean:
	touch a.out a.o
	rm -f *.out core ${BOOTS} ${LOADERS} bootblk *.o errs \
		makefile.bak

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

#
# Specific rules
#

print:
	@pr Makefile
	@pr *.h *.c *.s

bootblk: bootblk.o
	strip bootblk.o; dd if=bootblk.o bs=32 skip=1 of=b.out 2> /dev/null
	dd if=b.out of=bootblk conv=sync count=1 2> /dev/null

bootblk.o: bootblk.c
	- cc -E ${INCLUDE} bootblk.c | as -o bootblk.o

qio.o: qio.c
	cc -E ${INCLUDE} qio.c | as -o qio.o

qdio.o: qdio.c
	cc -E ${INCLUDE} qdio.c | as -o qdio.o

cons60.o: cons60.c
	cc -E ${INCLUDE} cons60.c | as -o cons60.o

cons640.o: cons640.c
	cc -E ${INCLUDE} cons640.c | as -o cons640.o

cons650.o: cons650.c
	cc -E ${INCLUDE} cons650.c | as -o cons650.o

vaxstart.o: vaxstart.c
	cc -E ${INCLUDE} vaxstart.c | as -o vaxstart.o

common.o: common.c
	cc -E ${INCLUDE} common.c | as -o common.o

common.SECONDARY.o: common.c
	cc -E -DSECONDARY ${INCLUDE} common.c | as -o common.SECONDARY.o

ultrixstart.o: ultrixstart.c
	cc -E ${INCLUDE} ultrixstart.c | as -o ultrixstart.o

pcsload.o: pcsload.c
	cc -E ${INCLUDE} pcsload.c | as -o pcsload.o

emulate.o: emulate.c
	cc -E ${INCLUDE} emulate.c | as -o emulate.o

loadimage.o loadimage.SECONDARY.o: loadimage.c
	cc -c ${CFLAGS} -DSECONDARY loadimage.c
	mv loadimage.o loadimage.SECONDARY.o
	cc -c ${CFLAGS} loadimage.c

io.o io.SECONDARY.o: ../io.c
	cc -c ${CFLAGS} -DSECONDARY ../io.c
	mv io.o io.SECONDARY.o
	cc -c ${CFLAGS} ../io.c

vaxstring.o: ../../machine/vax/emul/vaxstring.s
	/lib/cpp ${INCLUDE} -DMVAX ../../machine/vax/emul/vaxstring.s | as -o vaxstring.o

qfont.o: ../../io/uba/qfont.c
	cc ${CFLAGS} -c ../../io/uba/qfont.c

version.c: FRC
	echo -n "char *version=\"" > version.c; \
	echo -n "V$(RELEASE) " `date` \
		   >> version.c; \
	echo "\";" >> version.c

version.o: version.c
	-rm -f version.o
	cc $(CFLAGS) -c version.c

FRC:

sys.o sys.SECONDARY.o: ../sys.c
	cc -c ${CFLAGS} -DSECONDARY ../sys.c
	mv sys.o sys.SECONDARY.o
	cc -c ${CFLAGS} ../sys.c

decompress.o: ../decompress.c
	cc ${CFLAGS} -c ../decompress.c

ultrixboot.o: ../ultrixboot.c
	cc ${CFLAGS} -c ../ultrixboot.c

ultrixload.o: ../ultrixload.c
	cc ${CFLAGS} -c ../ultrixload.c

netload.o: ../netload.c
	cc ${CFLAGS} -c ../netload.c

vaxboot.o: vaxboot.c
	cc ${CFLAGS} -c vaxboot.c

#
# VAX boot image which resides in LBNs 0-15
#
bootblks: bootblk vaxstart.o common.o vaxboot.o io.o \
		sys.o qio.o loadimage.o
	ld -T ${VAX_RELOC} -N vaxstart.o common.o vaxboot.o \
		io.o sys.o qio.o loadimage.o -lc
	strip a.out;dd if=a.out of=b.out ibs=32 skip=1 2> /dev/null
	cat bootblk b.out > bootblks

#
# ultrixboot rules
#
ultrixboot: ultrixstart.o common.SECONDARY.o pcsload.o ultrixboot.o \
		sys.SECONDARY.o qdcons.o qdio.o cons60.o cons640.o \
		cons650.o qvcons.o qfont.o io.SECONDARY.o qio.o \
		get.o prf.o geterr.o emulate.o \
		loadimage.SECONDARY.o vaxstring.o
	make version.o; ld -T ${VAX_RELOC} -N -o ultrixboot \
		ultrixstart.o \
		common.SECONDARY.o pcsload.o ultrixboot.o \
		sys.SECONDARY.o qdcons.o qdio.o cons60.o cons640.o \
		cons650.o qvcons.o qfont.o io.SECONDARY.o qio.o \
		get.o prf.o geterr.o emulate.o \
		loadimage.SECONDARY.o vaxstring.o version.o -lc

#
# ultrixload rules
#
ultrixload: bootblk vaxstart.o common.SECONDARY.o ultrixload.o \
		qdcons.o qdio.o cons60.o cons640.o cons650.o \
		qvcons.o qfont.o qio.o get.o prf.o io.SECONDARY.o \
		decompress.o geterr.o emulate.o vaxstring.o
	make version.o; ld -T ${VAX_RELOC} -N vaxstart.o \
		common.SECONDARY.o ultrixload.o \
		qdcons.o qdio.o cons60.o cons640.o cons650.o \
		qvcons.o qfont.o qio.o get.o prf.o io.SECONDARY.o \
		decompress.o geterr.o emulate.o vaxstring.o version.o -lc
	strip a.out;dd if=a.out of=b.out ibs=32 skip=1 2> /dev/null
	cat bootblk b.out > a.out
	dd if=a.out of=ultrixload conv=sync 2> /dev/null
	rm -f a.out b.out

#
# netload rules
#
netload: vaxstart.o common.SECONDARY.o netload.o\
		qdcons.o qdio.o cons60.o cons640.o cons650.o\
		qvcons.o qfont.o qio.o prf.o\
		emulate.o vaxstring.o
	make version.o; ld -T ${VAX_RELOC} -N -o netload \
		vaxstart.o common.SECONDARY.o netload.o\
		qdcons.o qdio.o cons60.o cons640.o cons650.o\
		qvcons.o qfont.o qio.o prf.o\
		emulate.o vaxstring.o version.o -lc

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

