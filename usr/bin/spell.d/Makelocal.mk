# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90
include $(GMAKEVARS)
CINCLUDES=-I..
OBJS=spell.o spellin.o spellout.o

all:	spell hlist hlista hlistb hstop spellin spellout

install:
	-if [ ! -d ${DESTROOT}/usr/dict ];\
	then \
		mkdir ${DESTROOT}/usr/dict; \
		/etc/chown root ${DESTROOT}/usr/dict; \
		chgrp system ${DESTROOT}/usr/dict; \
		chmod 0755 ${DESTROOT}/usr/dict; \
	else true; \
	fi
	install -c -s spell ${DESTROOT}/usr/lib/spell
	install -c -s spellin ${DESTROOT}/usr/bin/spellin
	install -c -s spellout ${DESTROOT}/usr/bin/spellout
	install -c hlista ${DESTROOT}/usr/dict/hlista
	install -c hlistb ${DESTROOT}/usr/dict/hlistb
	install -c hstop ${DESTROOT}/usr/dict/hstop
	install -c ../spell.sh ${DESTROOT}/usr/bin/spell
	install -c ../words ${DESTROOT}/usr/dict/words
	install -c /dev/null ${DESTROOT}/usr/dict/spellhist

spell.o: spell.c
spellin.o: spellin.c
spellout.o: spellout.c

spell: spell.o
	$(LDCMD) spell.o
spellin: spellin.o
	$(LDCMD) spellin.o
spellout: spellout.o
	$(LDCMD) spellout.o

hlist: ../words spellin
	spellin <../words >hlist
hlista: ../american ../local hlist spellin
	(cat ../american ../local)|spellin hlist >hlista
hlistb: ../british ../local hlist spellin
	(cat ../british ../local)|spellin hlist >hlistb
hstop: ../stop spellin
	spellin <../stop >hstop
include $(GMAKERULES)
