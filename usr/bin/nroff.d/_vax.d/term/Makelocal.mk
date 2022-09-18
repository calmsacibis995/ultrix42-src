#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

# Possible options:
#	make		compile source for terminal tables below
#	make <term>	compile table for a specific <term>
#	make install	move tables to ${DEST}
#   	make link_copies   make link_copies for ease of documentation
#	make clean	remove tab*.o files lying around

DESTLIST=${DESTROOT}/usr/src/usr.bin/nroff/term ${DESTROOT}/usr/lib/term

ALL=	37 lpr 300 300-12 302 302-12 382 382-12 450 450-12 833 833-12 \
	itoh itoh12 nec nec12 nec-t qume qume12 xerox xerox12 \
	x-ecs x-ecs12 ln01 ln03 lqp02 lqp02-12

DEST=	${DESTROOT}/usr/src/usr.bin/nroff/term
DESTI=	${DESTROOT}/usr/lib/term

all:	${ALL}

37:	tab37.o
lpr:	tablpr.o
300:	tab300.o code.300
300-12:	tab300-12.o code.300
302:	tab302.o code.300
302-12:	tab302-12.o code.300
382:	tab382.o code.300
382-12: tab382-12.o code.300
450:	tab450.o code.300
450-12:	tab450-12.o code.300
833:	tab833.o code.aj833
833-12: tab833-12.o code.aj833
itoh:	tabitoh.o code.itoh
itoh12: tabitoh12.o code.itoh
nec:	tabnec.o code.nec
nec12:	tabnec12.o code.nec
nec-t:	tabnec-t.o
qume:	tabqume.o
qume12:	tabqume12.o
xerox:	tabxerox.o code.xerox
xerox12:tabxerox12.o code.xerox
x-ecs:	tabx-ecs.o code.x-ecs
x-ecs12:tabx-ecs12.o code.x-ecs
ln01: tabln01.o
ln03: tabln03.o
lqp02: tablqp02.o
lqp02-12: tablqp02-12.o

tab300-12.o:	tab300-12.c
tab300.o:	tab300.c
tab302-12.o:	tab302-12.c
tab302.o:	tab302.c
tab37.o:	tab37.c
tab382-12.o:	tab382-12.c
tab382.o:	tab382.c
tab450-12.o:	tab450-12.c
tab450.o:	tab450.c
tab833-12.o:	tab833-12.c
tab833.o:	tab833.c
tabitoh.o:	tabitoh.c
tabitoh12.o:	tabitoh12.c
tabln01.o:	tabln01.c
tabln03.o:	tabln03.c
tablpr.o:	tablpr.c
tablqp02-12.o:	tablqp02-12.c
tablqp02.o:	tablqp02.c
tabnec-t.o:	tabnec-t.c
tabnec.o:	tabnec.c
tabnec12.o:	tabnec12.c
tabqume.o:	tabqume.c
tabqume12.o:	tabqume12.c
tabx-ecs.o:	tabx-ecs.c
tabx-ecs12.o:	tabx-ecs12.c
tabxerox.o:	tabxerox.c
tabxerox12.o:	tabxerox12.c

install:
	@for file in ../tab*.c; do \
		$(ECHO) "$(INSTALL) -c $$file ${DEST}/`basename $$file`";\
		$(INSTALL) -c $$file ${DEST}/`basename $$file`;\
	done
	@for file in ../code.*; do \
		$(ECHO) "$(INSTALL) -c $$file ${DEST}/`basename $$file`";\
		$(INSTALL) -c $$file ${DEST}/`basename $$file`;\
	done
	$(INSTALL) -c ../Makefile.install ${DEST}/Makefile
	$(INSTALL) -c ../chartst ${DEST}/chartst
	$(RM) ${DEST}/tabnec-t
	@for file in tab*.o; do \
		$(ECHO) "$(INSTALL) -c -m 755 $$file ${DESTI}/`basename $$file .o`";\
		$(INSTALL) -c -m 755 $$file ${DESTI}/`basename $$file .o`;\
	done
	$(RM) ${DESTI}/tabtn300;
	$(LN) ${DESTI}/tablpr ${DESTI}/tabtn300
	$(RM) ${DESTI}/tabcrt;
	$(LN) ${DESTI}/tablpr ${DESTI}/tabcrt
	$(RM) ${DESTI}/tabvt100;
	$(LN) ${DESTI}/tablpr ${DESTI}/tabvt100
	$(RM) ${DESTI}/tab300s;
	$(LN) ${DESTI}/tab302 ${DESTI}/tab300s
	$(RM) ${DESTI}/tab300s-12;
	$(LN) ${DESTI}/tab302-12 ${DESTI}/tab300s-12
	$(RM) ${DESTI}/tabdtc;
	$(LN) ${DESTI}/tab302-12 ${DESTI}/tabdtc
	$(RM) ${DESTI}/tabdtc12;
	$(LN) ${DESTI}/tab302-12 ${DESTI}/tabdtc12
	$(RM) ${DESTI}/tabipsi;
	$(LN) ${DESTI}/tab302-12 ${DESTI}/tabipsi
	$(RM) ${DESTI}/tabipsi12;
	$(LN) ${DESTI}/tab302-12 ${DESTI}/tabipsi12
	$(INSTALL) -c -m 644 ../README ${DEST}/README
	$(INSTALL) -c -m 644 ../README ${DESTI}/README

include $(GMAKERULES)
