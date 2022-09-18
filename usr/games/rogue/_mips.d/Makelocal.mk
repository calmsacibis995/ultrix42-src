#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	5.4 (Berkeley) 7/9/88
#

include $(GMAKEVARS)

DESTLIST= ${DESTROOT}/usr/games

CFLAGS=	-O -DUNIX -DUNIX_BSD4_2 -DCURSES

LOADLIBES= -lcurses -ltermlib

AOUT= rogue

OBJS=	curses.o hit.o init.o inventory.o level.o machdep.o main.o \
	message.o monster.o move.o object.o pack.o play.o random.o ring.o \
	room.o save.o score.o spec_hit.o throw.o trap.o use.o zap.o

all: rogue

install: FRC
	install -c -s -m 711 rogue ${DESTROOT}/usr/games/rogue

curses.o:	curses.c
hit.o:		hit.c
init.o:		init.c
inventory.o:	inventory.c
level.o:	level.c
machdep.o:	machdep.c
main.o:		main.c
message.o:	message.c
monster.o:	monster.c
move.o:		move.c
object.o:	object.c
pack.o:		pack.c
play.o:		play.c
random.o:	random.c
ring.o:		ring.c
room.o:		room.c
save.o:		save.c
score.o:	score.c
spec_hit.o:	spec_hit.c
throw.o:	throw.c
trap.o:		trap.c
use.o:		use.c
zap.o:		zap.c

include $(GMAKERULES)
