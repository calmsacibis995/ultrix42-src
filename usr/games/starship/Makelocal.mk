#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

CFLAGS= -O -DDRONE -DSHMEM
SFLAGS= -O -DDRONE
LOADLIBES=  -lcurses -ltermlib
OBJS=main.o cmd.o screen.o action.o drone.o starsleep.o
DESTLIST= ${DESTROOT}/usr/games

all:  starship rmstar singleship


starship: ${OBJS}
	cc ${CFLAGS} -o starship ${OBJS} -lcurses -ltermlib

singleship: ${OBJS}
	cc ${SFLAGS} -o singleship ${OBJS} -lcurses -ltermlib

rmstar: rmstar.o
	cc -o rmstar rmstar.o

install: starship rmstar
	install  -c -o daemon -s -m 4711 starship ${DESTROOT}/usr/games/starship
	install -c -o daemon -s -m 4711 rmstar ${DESTROOT}/usr/games/rmstar
	install -c -o daemon -m 744 ../README ${DESTROOT}/usr/games/README


main.o :	main.c
cmd.o :		cmd.c
screen.o :	screen.c
action.o :	action.c
drone.o :	drone.c
starsleep.o:	starsleep.c
rmstar.o:	rmstar.c

include $(GMAKERULES)
