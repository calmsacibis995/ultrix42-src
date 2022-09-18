# @(#)Makefile	1.8	ULTRIX	2/8/85
#
#	@(#)Makefile	1.8	(Berkeley)	7/2/83
#
M=../../../lib/vax/mip
LOAD1=
LOAD2=
CFLAGS=-O -DFLEXNAMES
LINTLIBS=llib-port.ln llib-lc.ln llib-lcV.ln llib-lm.ln llib-lmV.ln llib-lmp.ln llib-lcurses.ln
DESTROOT= 

all:	lpass1 lpass2 ${LINTLIBS}

lpass1: cgram.o xdefs.o scan.o comm1.o pftn.o trees.o optim.o lint.o hash.o
	${CC} cgram.o xdefs.o scan.o comm1.o pftn.o trees.o optim.o \
		lint.o hash.o -o lpass1

trees.o: $M/manifest macdefs $M/mfile1 $M/trees.c
	${CC} -c ${CFLAGS} -I$M -I. $M/trees.c

optim.o: $M/manifest macdefs $M/mfile1 $M/optim.c
	${CC} -c ${CFLAGS} -I$M -I. $M/optim.c

pftn.o: $M/manifest macdefs $M/mfile1 $M/pftn.c
	${CC} -c ${CFLAGS} -I$M -I. $M/pftn.c

lint.o: $M/manifest macdefs $M/mfile1 lmanifest
	${CC} -c ${CFLAGS} -I$M -I. lint.c

scan.o: $M/manifest macdefs $M/mfile1 $M/scan.c
	${CC} -c ${CFLAGS} -I$M -I. $M/scan.c

xdefs.o: $M/manifest $M/mfile1 macdefs $M/xdefs.c
	${CC} -c ${CFLAGS} -I$M -I. $M/xdefs.c

cgram.o: $M/manifest $M/mfile1 macdefs cgram.c
	${CC} -c ${CFLAGS} -I$M -I. cgram.c

cgram.c: $M/cgram.y
	yacc $M/cgram.y
	mv y.tab.c cgram.c

comm1.o: $M/manifest $M/mfile1 $M/common macdefs $M/comm1.c
	${CC} -c ${CFLAGS} -I. -I$M $M/comm1.c

llib-port.ln: llib-port lpass1
	-(/lib/cpp -C -Dlint llib-port | ./lpass1 -puv > llib-port.ln )

llib-lm.ln: llib-lm lpass1
	-(/lib/cpp -C -Dlint llib-lm | ./lpass1 -puv > llib-lm.ln )

llib-lmV.ln: llib-lm lpass1
	-(/lib/cpp -C -Dlint -DSYSTEM_FIVE llib-lm | ./lpass1 -puv > llib-lmV.ln )

llib-lmp.ln: llib-lmp lpass1
	-(/lib/cpp -C -Dlint llib-lmp | ./lpass1 -puv > llib-lmp.ln )

llib-lc.ln: llib-lc lpass1
	-(/lib/cpp -C -Dlint llib-lc | ./lpass1 -v > llib-lc.ln )

llib-lcV.ln: llib-lc lpass1
	-(/lib/cpp -C -Dlint -DSYSTEM_FIVE llib-lc | ./lpass1 -v > llib-lcV.ln )

llib-lcurses.ln: llib-lcurses lpass1
	-(/lib/cpp -C -Dlint llib-lcurses | ./lpass1 -v > llib-lcurses.ln )

lpass2: lpass2.o hash.o
	${CC} lpass2.o hash.o -o lpass2

lpass2.o: $M/manifest lmanifest
	${CC} ${CFLAGS} -c -I$M -I. lpass2.c
	
lintall:
	lint -hpv -I. -I$M  $M/cgram.c $M/xdefs.c $M/scan.c \
		$M/pftn.c $M/trees.c $M/optim.c lint.c

install: all SHELL
	@-if [ ! -d ${DESTROOT}/usr/lib/lint ] ; then \
		mkdir ${DESTROOT}/usr/lib/lint ; \
		/etc/chown root ${DESTROOT}/usr/lib/lint ; \
		chgrp system ${DESTROOT}/usr/lib/lint ; \
		chmod 0755 ${DESTROOT}/usr/lib/lint ; \
	fi
	install -c -s lpass1 ${DESTROOT}/usr/lib/lint/lint1
	install -c -s lpass2 ${DESTROOT}/usr/lib/lint/lint2
	for i in llib-*; do install -c -m 644 $$i ${DESTROOT}/usr/lib/lint; done
	install -c SHELL ${DESTROOT}/usr/bin/lint

tools2: all SHELL
	@-if [ ! -d ${DESTROOT}/usr/lib/lint ] ; then \
		mkdir ${DESTROOT}/usr/lib/lint ; \
		/etc/chown root ${DESTROOT}/usr/lib/lint ; \
		chgrp system ${DESTROOT}/usr/lib/lint ; \
		chmod 0755 ${DESTROOT}/usr/lib/lint ; \
	fi
	install -c -s lpass1 ${DESTROOT}/usr/lib/lint/lint1
	install -c -s lpass2 ${DESTROOT}/usr/lib/lint/lint2
	for i in llib-*; do install -c -m 644 $$i ${DESTROOT}/usr/lib/lint; done
	install -c SHELL ${DESTROOT}/usr/bin/lint

shrink:
	rm -f *.o

clean: shrink
	rm -f lpass1 lpass2 cgram.c ${LINTLIBS}

clobber: clean
	rm -f llib-* lmanifest macdefs SHELL *.[ch] Makefile READ_ME

sccsinfo:
	sccs info

sccsget:
	sccs get SCCS

.DEFAULT:
	sccs get $<
