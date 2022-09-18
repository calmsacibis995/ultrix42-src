#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
# BSD curses(3x) package

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib

CURSES	= libcurses.a

CFLAGS= -O $(DEBUG)

CFILES=	box.c clear.c initscr.c endwin.c mvprintw.c mvscanw.c mvwin.c \
	newwin.c overlay.c overwrite.c printw.c scanw.c refresh.c \
	touchwin.c erase.c clrtobot.c clrtoeol.c cr_put.c cr_tty.c \
	longname.c delwin.c insertln.c deleteln.c scroll.c getstr.c \
	getch.c addstr.c addch.c move.c curses.c unctrl.c standout.c \
	tstp.c insch.c delch.c

OBJECTS=	addch.o addstr.o box.o clear.o clrtobot.o clrtoeol.o cr_put.o \
	cr_tty.o curses.o delch.o deleteln.o delwin.o endwin.o erase.o \
	getch.o getstr.o initscr.o insch.o insertln.o longname.o move.o \
	mvprintw.o mvscanw.o mvwin.o newwin.o overlay.o overwrite.o \
	printw.o refresh.o scanw.o scroll.o standout.o touchwin.o tstp.o \
	unctrl.o


#
# include $(GMAKERULES) must appear ABOVE Makelocal_$(MACHINE).mk
# in order for the VAX version to work.
#
include $(GMAKERULES)
include ../Makelocal_$(MACHINE).mk

$(CURSES): $(OBJECTS)
	@echo building normal $(CURSES)
	@ar cr $(CURSES) $(OBJECTS)
	ranlib $(CURSES)

tools2:	all
tools2 install: $(DESTLIST) $(MACHINE)install
	$(INSTALL) -c -m 644 $(CURSES) $(DESTROOT)/usr/lib/libcurses.a
	ranlib $(DESTROOT)/usr/lib/libcurses.a


addch.o:	addch.c
addstr.o:	addstr.c
box.o:	box.c
clear.o:	clear.c
clrtobot.o:	clrtobot.c
clrtoeol.o:	clrtoeol.c
cr_put.o:	cr_put.c
cr_tty.o:	cr_tty.c
curses.o:	curses.c
delch.o:	delch.c
deleteln.o:	deleteln.c
delwin.o:	delwin.c
endwin.o:	endwin.c
erase.o:	erase.c
getch.o:	getch.c
getstr.o:	getstr.c
initscr.o:	initscr.c
insch.o:	insch.c
insertln.o:	insertln.c
longname.o:	longname.c
move.o:	move.c
mvprintw.o:	mvprintw.c
mvscanw.o:	mvscanw.c
mvwin.o:	mvwin.c
newwin.o:	newwin.c
overlay.o:	overlay.c
overwrite.o:	overwrite.c
printw.o:	printw.c
refresh.o:	refresh.c
scanw.o:	scanw.c
scroll.o:	scroll.c
standout.o:	standout.c
touchwin.o:	touchwin.c
tstp.o:	tstp.c
unctrl.o:	unctrl.c

