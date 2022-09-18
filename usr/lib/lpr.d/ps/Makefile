#  @(#)Makefile	4.1	ULTRIX	11/23/87
#
#	Makefile for PostScript(TM) Print filters
#

OBJS = imscript.o plscript.o txscript.o

INSTFLAGS=
CFLAGS= -g

LINT=
LINTFLAGS=

STD = imscript plscript txscript

PROLOG = improlog.h plprolog.h

DESTROOT =

PSDIR = /usr/lib/ps

all: 	${STD}

imscript: imscript.o 
	ld -X -o imscript /lib/crt0.o imscript.o -lc -lg

plscript: plscript.o
	ld -X -o plscript /lib/crt0.o plscript.o -lm -lc -lg 

txscript: txscript.o
	ld -X -o txscript /lib/crt0.o txscript.o -lc -lg

install: all

install: all
	-if [ ! -d ${PSDIR} ]; \
	 then \
		mkdir ${PSDIR}; \
		chmod 755 ${PSDIR}; \
		/etc/chown root ${PSDIR}; \
		chgrp system ${PSDIR}; \
	else true; \
	fi
	install ${INSTFLAGS} -c -m 755 imscript ${PSDIR}/imscript
	install ${INSTFLAGS} -c -m 755 plscript ${PSDIR}/plscript
	install ${INSTFLAGS} -c -m 755 txscript ${PSDIR}/txscript

clean:
	rm -f a.out core ${OBJS} ${STD} ${PROLOG}

clobber:	
	rm -f tags
	${SCCS} clean

sccsget:
	${SCCS} get ${DATE} SCCS

sccsinfo:
	@${SCCS} info

tags:
	ctags *.[ch]

lint:
	${LINT} ${LINTFLAGS} *.c

imscript.o: imscript.c improlog.h 
plscript.o: plscript.c plprolog.h 
txscript.o: txscript.c 
#
#	PostScript prolog filter setup
#
improlog.h: improlog.ps
	sed -e 's/.*/"&",/' improlog.ps > improlog.h
plprolog.h: plprolog.ps
	sed -e 's/.*/"&",/' plprolog.ps > plprolog.h

