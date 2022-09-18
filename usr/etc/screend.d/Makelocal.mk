# SCCSID: @(#)Makelocal.mk	4.2	ULTRIX	10/12/90

include $(GMAKEVARS)

DESTLIST=${DESTROOT}/usr/etc

AOUTS=	screend ckscreentab screentest screenmode screenstat screenmini
CINCLUDES = -I..
YFLAGS = -vd
LOADLIBES = -ll

PARSEOBJS = screentabl.o screentab.o nametoval.o buildtab.o printsubs.o acttab.o
SCREENOBJS = unpack.o screenit.o frag.o log.o
CKOBJS = ${PARSEOBJS} ckscreentab.o
SCREENDOBJS = ${PARSEOBJS} ${SCREENOBJS} screend.o
TESTOBJS = screentest.o printsubs.o nametoval.o

acttab.o: acttab.c screentab.h
buildtab.o: buildtab.c screentab.h
ckscreentab.o: ckscreentab.c
frag.o: frag.c
log.o: log.c
nametoval.o: nametoval.c
printsubs.o: printsubs.c screentab.h
screend.o: screend.c screentab.h
screenit.o: screenit.c screentab.h
screenmini.o: screenmini.c
screenmode.o: screenmode.c
screenstat.o: screenstat.c
screentab.o: screentab.y
screentabl.o: screentab.o screentabl.l
screentest.o: screentest.c
unpack.o: unpack.c screentab.h

screend: ${SCREENDOBJS}
ckscreentab: ${CKOBJS}
screentest: ${TESTOBJS}
screenmode: screenmode.o
screenstat: screenstat.o
screenmini: screenmini.o

install:
	$(INSTALL) -s -c screend $(DESTROOT)/usr/etc/screend
	$(INSTALL) -s -c screenmode $(DESTROOT)/usr/etc/screenmode
	$(INSTALL) -s -c screenstat $(DESTROOT)/usr/etc/screenstat

# These are test programs only, they do not ship
#	$(INSTALL) -s -c ckscreentab $(DESTROOT)/usr/bin/ckscreentab
#	$(INSTALL) -s -c screentest $(DESTROOT)/usr/bin/screentest
#	$(INSTALL) -s -c screenmini $(DESTROOT)/usr/bin/screenmini

include $(GMAKERULES)
