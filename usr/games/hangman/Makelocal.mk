#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/usr/games
AOUT= hangman

OBJS=	endgame.o extern.o getguess.o getword.o main.o playgame.o \
	prdata.o prman.o prword.o setup.o

LOADLIBES= -lcurses -ltermlib

endgame.o:	endgame.c
extern.o :	extern.c
getguess.o:	getguess.c
getword.o :	getword.c
main.o :	main.c
playgame.o :	playgame.c
prdata.o :	prdata.c
prman.o :	prman.c
prword.o :	prword.c
setup.o:	setup.c

install:
	$(INSTALL) -c -s hangman $(DESTROOT)/usr/examples/hangman
	$(LN) -s /usr/examples/hangman $(DESTROOT)/usr/games/hangman

include $(GMAKERULES)
