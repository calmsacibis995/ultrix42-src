#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DFILES=	driver1.c driver2.c driver3.c grap.c
DOBJS=	driver1.o driver2.o driver3.o grap.o
PFILES=	version.c player1.c player2.c
POBJS=	version.o player1.o player2.o
LFILES=	sail.log.c
LOBJS=	sail.log.o
COMMONFILES= assorted.c boarders.c game.c globals.c\
	main.c parties.c screen.c machdep.c
COMMONOBJS= assorted.o boarders.o game.o globals.o\
	main.o parties.o screen.o machdep.o
HDRS=	externs.h player.h machdep.h
OTHERFILES= Makefile sail.m
CTAGFILES= ${DFILES} ${PFILES} ${LFILES} ${COMMONFILES}
JUNKFILES= sail driver sail.log sail.doc junk arch tags
PLIBS= 	-lcurses -ltermlib
DLIBS=
LLIBS=
SAILMASTER= daemon
SAIL=	sail
DRIVER=	lib/saildriver
SAIL.LOG= sail.log
SAIL.DOC= sail.doc
SHIPSLOG= lib/saillog
DESTROOT=
TARGET=	${DESTROOT}/usr/games
DESTLIST= ${TARGET} ${TARGET}/lib

all: sail driver sail.log sail.doc

sail: ${COMMONOBJS} ${POBJS}
	cc -O ${COMMONOBJS} ${POBJS} ${PLIBS} -o sail

driver: ${COMMONOBJS} ${DOBJS}
	cc -O ${COMMONOBJS} ${DOBJS} ${DLIBS} -o driver

${DOBJS} ${POBJS} ${LOBJS} ${COMMONOBJS}: externs.h machdep.h

sail.log: ${LOBJS} globals.o
	cc -O ${LOBJS} globals.o -o sail.log

sail.doc: sail.m
#	nroff -man sail.m > sail.doc


install: ${DESTLIST} ${TARGET}/${SHIPSLOG}
	install -c -s -m 4711 -o ${SAILMASTER} sail ${TARGET}/${SAIL}
	install -c -s -m 4711 -o ${SAILMASTER} driver ${TARGET}/${DRIVER}
	install -c -s -m 711 sail.log ${TARGET}/${SAIL.LOG}
#	cp sail.doc ${TARGET}/${SAIL.DOC}

${TARGET}/${SHIPSLOG}:
	cp /dev/null ${TARGET}/${SHIPSLOG}
	/etc/chown ${SAILMASTER} ${TARGET}/${SHIPSLOG}
	chmod 644 ${TARGET}/${SHIPSLOG}

assorted.o :	assorted.c
	$(CC) -c ../assorted.c

boarders.o :	boarders.c
	$(CC) -c ../boarders.c

game.o :	game.c
	$(CC) -c ../game.c

globals.o:	globals.c
	$(CC) -c ../globals.c

main.o :	main.c
	$(CC) -c ../main.c

parties.o :	parties.c
	$(CC) -c ../parties.c

screen.o :	screen.c
	$(CC) -c ../screen.c

machdep.o:	machdep.c
	$(CC) -c ../machdep.c

version.o:	version.c
	$(CC) -c ../version.c

player1.o:	player1.c
	$(CC) -c ../player1.c

player2.o:	player2.c
	$(CC) -c ../player2.c

driver1.o:	driver1.c
	$(CC) -c ../driver1.c

driver2.o:	driver2.c
	$(CC) -c ../driver2.c

driver3.o:	driver3.c
	$(CC) -c ../driver3.c

grap.o:	grap.c
	$(CC) -c ../grap.c

sail.log.o:	sail.log.c
	$(CC) -c ../sail.log.c

include $(GMAKERULES)
