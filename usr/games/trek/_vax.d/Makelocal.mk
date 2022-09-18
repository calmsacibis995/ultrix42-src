#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

AOUT=	trek

OBJS=	abandon.o attack.o autover.o capture.o check_out.o checkcond.o \
	compkl.o computer.o damage.o damaged.o dcrept.o destruct.o \
	dock.o dumpgame.o dumpme.o dumpssradio.o events.o externs.o \
	getcodi.o getpar.o help.o impulse.o initquad.o kill.o klmove.o \
	lose.o lrscan.o main.o move.o nova.o out.o phaser.o play.o ram.o \
	ranf.o rest.o schedule.o score.o setup.o setwarp.o shell.o \
	shield.o snova.o srscan.o systemname.o torped.o utility.o \
	visual.o warp.o win.o cgetc.o

HFILES=	getpar.h trek.h

LOADLIBES= -lm


install: trek
	install -s trek ${DESTROOT}/usr/games/trek

abandon.o: abandon.c
abandon.o: trek.h
attack.o: attack.c
attack.o: trek.h
autover.o: autover.c
autover.o: trek.h
capture.o: capture.c
capture.o: trek.h
check_out.o: check_out.c
check_out.o: trek.h
checkcond.o: checkcond.c
checkcond.o: trek.h
compkl.o: compkl.c
compkl.o: trek.h
computer.o: computer.c
computer.o: trek.h
computer.o: getpar.h
computer.o: /usr/include/stdio.h
damage.o: damage.c
damage.o: trek.h
damaged.o: damaged.c
damaged.o: trek.h
dcrept.o: dcrept.c
dcrept.o: trek.h
destruct.o: destruct.c
destruct.o: trek.h
dock.o: dock.c
dock.o: trek.h
dumpgame.o: dumpgame.c
dumpgame.o: trek.h
dumpme.o: dumpme.c
dumpme.o: trek.h
dumpssradio.o: dumpssradio.c
dumpssradio.o: trek.h
events.o: events.c
events.o: trek.h
externs.o: externs.c
externs.o: trek.h
getcodi.o: getcodi.c
getcodi.o: getpar.h
getpar.o: getpar.c
getpar.o: /usr/include/stdio.h
getpar.o: getpar.h
help.o: help.c
help.o: trek.h
impulse.o: impulse.c
impulse.o: trek.h
initquad.o: initquad.c
initquad.o: trek.h
kill.o: kill.c
kill.o: trek.h
klmove.o: klmove.c
klmove.o: trek.h
lose.o: lose.c
lose.o: trek.h
lrscan.o: lrscan.c
lrscan.o: trek.h
main.o: main.c
main.o: trek.h
main.o: /usr/include/stdio.h
main.o: /usr/include/sgtty.h
move.o: move.c
move.o: trek.h
nova.o: nova.c
nova.o: trek.h
out.o: out.c
out.o: trek.h
phaser.o: phaser.c
phaser.o: trek.h
phaser.o: getpar.h
play.o: play.c
play.o: trek.h
play.o: getpar.h
ram.o: ram.c
ram.o: trek.h
ranf.o: ranf.c
ranf.o: /usr/include/stdio.h
rest.o: rest.c
rest.o: trek.h
rest.o: getpar.h
schedule.o: schedule.c
schedule.o: trek.h
score.o: score.c
score.o: trek.h
score.o: getpar.h
setup.o: setup.c
setup.o: trek.h
setup.o: getpar.h
setwarp.o: setwarp.c
setwarp.o: trek.h
setwarp.o: getpar.h
shell.o: shell.c
shield.o: shield.c
shield.o: trek.h
shield.o: getpar.h
snova.o: snova.c
snova.o: trek.h
srscan.o: srscan.c
srscan.o: trek.h
srscan.o: getpar.h
systemname.o: systemname.c
systemname.o: trek.h
torped.o: torped.c
torped.o: /usr/include/stdio.h
torped.o: trek.h
utility.o: utility.c
visual.o: visual.c
visual.o: trek.h
warp.o: warp.c
warp.o: trek.h
win.o: win.c
win.o: trek.h
win.o: getpar.h
cgetc.o: cgetc.c
cgetc.o: /usr/include/stdio.h

include $(GMAKERULES)
