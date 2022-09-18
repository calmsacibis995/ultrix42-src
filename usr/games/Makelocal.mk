#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

SUBDIRS= adventure backgammon boggle chess ching compat cribbage doctor \
	 fortune hangman mille monop quiz rogue sail snake starship\
	 trek zork

AOUT_GAMES=  arithmetic btlgammon banner bcd cfscores factor \
	fish number wump wargames\
	canfield primes rain worm worms

all:  $(AOUT_GAMES)

# do install2 before doing SUBDIRS
install: install2

install2:
	-if [ ! -d ${DESTROOT}/usr/games ]; then \
		mkdir ${DESTROOT}/usr/games; \
		chmod 755 ${DESTROOT}/usr/games; \
		/etc/chown root ${DESTROOT}/usr/games; \
		chgrp system ${DESTROOT}/usr/games; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/games/lib ]; \
		then \
			mkdir ${DESTROOT}/usr/games/lib; \
			chmod 755 ${DESTROOT}/usr/games/lib; \
			/etc/chown root ${DESTROOT}/usr/games/lib; \
			chgrp system ${DESTROOT}/usr/games/lib; \
	else true; \
	fi
	for i in ${AOUT_GAMES}; do \
		(install -c -s $$i ${DESTROOT}/usr/games/$$i); \
	done
	cat >${DESTROOT}/usr/games/lib/cfscores </dev/null
	chmod 777 ${DESTROOT}/usr/games/lib/cfscores

canfield: canfield.c 
	cc -o canfield ${CFLAGS} ../canfield.c -lcurses -ltermcap

primes: primes.c
	cc -o primes ${CFLAGS} ../primes.c -lm

rain: rain.c
	cc -o rain ${CFLAGS} ../rain.c -lcurses -ltermcap

worm: worm.c
	cc -o worm ${CFLAGS} ../worm.c -lcurses -ltermcap

worms: worms.c
	cc -o worms ${CFLAGS} ../worms.c -lcurses -ltermcap

arithmetic: arithmetic.c
	cc -o arithmetic ${CFLAGS} ../arithmetic.c

btlgammon: btlgammon.c
	cc -o btlgammon ${CFLAGS} ../btlgammon.c

banner: banner.c
	cc -o banner ${CFLAGS} ../banner.c

bcd: bcd.c
	cc -o bcd ${CFLAGS} ../bcd.c

cfscores: cfscores.c
	cc -o cfscores ${CFLAGS} ../cfscores.c

fish: fish.c
	cc -o fish ${CFLAGS} ../fish.c

factor: factor.c
	cc -o factor ${CFLAGS} ../factor.c

number: number.c
	cc -o number ${CFLAGS} ../number.c

wargames: wargames.c
	cc -o wargames ${CFLAGS} ../wargames.c

wump: wump.c
	cc -o wump ${CFLAGS} ../wump.c

include $(GMAKERULES)
