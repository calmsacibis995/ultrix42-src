#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/include $(DESTROOT)/usr/lib $(DESTROOT)/usr/bin

#	Curses Library Low Level Makefile
#
#	CFLAGS has -DVIDEO & -DKEYPAD for the libcurses.a library.  They could
#		be put into a separate variable if desired, but there is no
#		reason for doing so.
#	DFLAGS, TFLAGS, & PFLAGS are for debugging, tracing, & profiling
#		respectively.  They all contain the DEBUG flag because it is
#		thought that any of the styles of compiling is still debugging.
#		Also the DEBUG flag will create very large files and it is not
#		recommended that all of the files be compiled with the DEBUG
#		flag defined.  This will make 'ar' die because the library will
#		exceed 2065 blocks in size.  But there is no reason not to
#		compile	individual or groups of files with these flags defined.
#	FILES is used for the naming of the object files.
#	O is used to destinguish between compiling, debugging, tracing, &
#		profiling.
#	MINICURSES is a flag used to compile a small version of libcurses.
#		It should not be used for the compiling of libcurses.a.  It is
#		strictly for the application programmers convenience.
#		If MINICURSES is desired with ti4 or show then type:
#			"make <[ti4][show] MINICURSES=-DMINICURSES"
#
CURSES		= libcurses.a
O		= o
MINICURSES	=
CFLAGS		= -DVIDEO -DKEYPAD $(DEBUG) -O
DFLAGS		= -DVIDEO -DKEYPAD -DDEBUG
TFLAGS		= -DVIDEO -DKEYPAD -DDEBUG -O
PFLAGS		= -DVIDEO -DKEYPAD -DDEBUG -p -O
FILES		= __cflush.$(O) __sscans.$(O) _blanks.$(O) _c_clean.$(O) \
		_clearhl.$(O) _clearline.$(O) _comphash.$(O) _delay.$(O) \
		_delchars.$(O) _dellines.$(O) _dumpwin.$(O) _ec_quit.$(O) \
		_fixdelay.$(O) _forcehl.$(O) _hlmode.$(O) _id_char.$(O) \
		_init_cost.$(O) _inschars.$(O) _insmode.$(O) _kpmode.$(O) \
		_line_free.$(O) _ll_move.$(O) _outch.$(O) _outchar.$(O) \
		_pos.$(O) _reset.$(O) _scrdown.$(O) _scrollf.$(O) _sethl.$(O) \
		_setmode.$(O) _setwind.$(O) _shove.$(O) _sprintw.$(O) \
		_sputc.$(O) _syncmodes.$(O) _tscroll.$(O) _window.$(O) \
		addch.$(O) addstr.$(O) baudrate.$(O) beep.$(O) box.$(O) \
		capnames.$(O) cbreak.$(O) chktypeahd.$(O) clear.$(O) \
		clearok.$(O) clreolinln.$(O) clrtobot.$(O) clrtoeol.$(O) \
		cntcostfn.$(O) crmode.$(O) curses.$(O) def_prog.$(O) \
		def_shell.$(O) delayoutpt.$(O) delch.$(O) deleteln.$(O) \
		delwin.$(O) doprnt.$(O) doscan.$(O) doupdate.$(O) draino.$(O) \
		echo.$(O) endwin.$(O) erase.$(O) erasechar.$(O) fixterm.$(O) \
		flash.$(O) flushinp.$(O) getch.$(O) getstr.$(O) idlok.$(O) \
		gettmode.$(O) has_ic.$(O) has_il.$(O) idln.getst.$(O) \
		initkeypad.$(O) initscr.$(O) insch.$(O) insertln.$(O) \
		intrflush.$(O) keypad.$(O) killchar.$(O) leaveok.$(O) \
		line_alloc.$(O) ll_refresh.$(O) longname.$(O) m_addch.$(O) \
		m_addstr.$(O) m_clear.$(O) m_erase.$(O) m_move.$(O) meta.$(O) \
		m_refresh.$(O) m_tstp.$(O) makenew.$(O) miniinit.$(O) \
		move.$(O) mvcur.$(O) mvprintw.$(O) mvscanw.$(O) mvwin.$(O) \
		mvwprintw.$(O) mvwscanw.$(O) naps.$(O) newpad.$(O) \
		newterm.$(O) newwin.$(O) nl.$(O) nocbreak.$(O) nocrmode.$(O) \
		nodelay.$(O) noecho.$(O) nonl.$(O) noraw.$(O) nttychktrm.$(O) \
		overlay.$(O) overwrite.$(O) pnoutrfrsh.$(O) prefresh.$(O) \
		printw.$(O) putp.$(O) raw.$(O) reset_prog.$(O) resetshell.$(O) \
		resetterm.$(O) resetty.$(O) restarttrm.$(O) saveterm.$(O) \
		savetty.$(O) scanw.$(O) scroll.$(O) scrollok.$(O) \
		set_term.$(O) setbuffred.$(O) setterm.$(O) setupterm.$(O) \
		showstring.$(O) subwin.$(O) tgetent.$(O) tgetflag.$(O) \
		tgetnum.$(O) tgetstr.$(O) tgoto.$(O) touchwin.$(O) tparm.$(O) \
		tputs.$(O) traceonoff.$(O) tstp.$(O) two.twostr.$(O) \
		typeahead.$(O) unctrl.$(O) vidattr.$(O) vidputs.$(O) \
		vsprintf.$(O) vsscanf.$(O) wattroff.$(O) wattron.$(O) \
		wattrset.$(O) wnoutrfrsh.$(O) wprintw.$(O) wrefresh.$(O) \
		writechars.$(O) wscanw.$(O) wstandend.$(O) wstandout.$(O)

.SUFFIXES:	.o .d .t .p

all:		$(CURSES) tic termcap.o

tic:		tic.o capnames.o
		$(CC) $(CFLAGS) $(MINICURSES) tic.o capnames.o -o tic

tic.o:		tic.c
termcap.o:	termcap.c

$(CURSES):	$(FILES) curses.h curshdr.h term.h unctrl.h curses.ext curses.c
		ar rv $(CURSES) $(FILES)
		ranlib $(CURSES)

OLDdebug:
		$(MAKE) O=d CURSES=libdcurses.a

OLDtrace:
		$(MAKE) O=t CURSES=libtcurses.a

OLDprofile:
		$(MAKE) O=p CURSES=libpcurses.a

OLDshow:		libcurses.a show.o
		$(CC) $(CFLAGS) $(MINICURSES) show.o libcurses.a -o show
		/bin/rm show.o


OLDti4:		libcurses.a ti4.o
		$(CC) $(CFLAGS) $(MINICURSES) ti4$(O) libcurses.a -o ti4
		/bin/rm ti4.o

OLDti4.o:
		cc mkti4.c -O -o mkti4
		mkti4 >ti4.c
		cc $(CFLAGS) $(MINICURSES) ti4.c -O -c

pretools tools1:
	$(INSTALL) -c -m 444 ../curses.h $(DESTROOT)/usr/include/cursesX.h

tools2:	$(CURSES)
tools2 install:
	$(INSTALL) -c -m 444 ../curses.h $(DESTROOT)/usr/include/cursesX.h
	$(INSTALL) -c -m 644 termcap.o $(DESTROOT)/usr/lib/termcap.o
	$(INSTALL) -c -m 644 $(CURSES) $(DESTROOT)/usr/lib/libcursesX.a
	ranlib $(DESTROOT)/usr/lib/libcursesX.a
	$(INSTALL) -c -s tic $(DESTROOT)/usr/bin/tic

term.h:
		ex < maketerm.ex

OLDtermcap:
		ed < termcap.ed
		rm -f /tmp/caps
		cc -c termcap.c

include $(GMAKERULES)
.c.o:
			$(CCCMD) $(MINICURSES) ../$*.c

.c.d:
			if [ -f $*.o ] ; then mv $*.o tmp ; fi
			$(CC) $(DFLAGS) $(MINICURSES) -c ../$*.c
			mv $*.o $*.d
			if [ -f tmp ] ; then mv tmp $*.o
.c.t:
			if [ -f $*.o ] ; then mv $*.o tmp ; fi
			$(CC) $(TFLAGS) $(MINICURSES) -c ../$*.c
			mv $*.o $*.d
			if [ -f tmp ] ; then mv tmp $*.o

.c.p:
			if [ -f $*.o ] ; then mv $*.o tmp ; fi
			$(CC) $(PFLAGS) $(MINICURSES) -c ../$*.c
			mv $*.o $*.d
			if [ -f tmp ] ; then mv tmp $*.o

__cflush.o:	__cflush.c
__sscans.o:	__sscans.c
_blanks.o:	_blanks.c
_c_clean.o:	_c_clean.c
_clearhl.o:	_clearhl.c
_clearline.o:	_clearline.c
_comphash.o:	_comphash.c
_delay.o:	_delay.c
_delchars.o:	_delchars.c
_dellines.o:	_dellines.c
_dumpwin.o:	_dumpwin.c
_ec_quit.o:	_ec_quit.c
_fixdelay.o:	_fixdelay.c
_forcehl.o:	_forcehl.c
_hlmode.o:	_hlmode.c
_id_char.o:	_id_char.c
_init_cost.o:	_init_cost.c
_inschars.o:	_inschars.c
_insmode.o:	_insmode.c
_kpmode.o:	_kpmode.c
_line_free.o:	_line_free.c
_ll_move.o:	_ll_move.c
_outch.o:	_outch.c
_outchar.o:	_outchar.c
_pos.o:	_pos.c
_reset.o:	_reset.c
_scrdown.o:	_scrdown.c
_scrollf.o:	_scrollf.c
_sethl.o:	_sethl.c
_setmode.o:	_setmode.c
_setwind.o:	_setwind.c
_shove.o:	_shove.c
_sprintw.o:	_sprintw.c
_sputc.o:	_sputc.c
_syncmodes.o:	_syncmodes.c
_tscroll.o:	_tscroll.c
_window.o:	_window.c
addch.o:	addch.c
addstr.o:	addstr.c
baudrate.o:	baudrate.c
beep.o:	beep.c
box.o:	box.c
capnames.o:	capnames.c
cbreak.o:	cbreak.c
chktypeahd.o:	chktypeahd.c
clear.o:	clear.c
clearok.o:	clearok.c
clreolinln.o:	clreolinln.c
clrtobot.o:	clrtobot.c
clrtoeol.o:	clrtoeol.c
cntcostfn.o:	cntcostfn.c
crmode.o:	crmode.c
curses.o:	curses.c
def_prog.o:	def_prog.c
def_shell.o:	def_shell.c
delayoutpt.o:	delayoutpt.c
delch.o:	delch.c
deleteln.o:	deleteln.c
delwin.o:	delwin.c
doprnt.o:	doprnt.c
doscan.o:	doscan.c
doupdate.o:	doupdate.c
draino.o:	draino.c
echo.o:	echo.c
endwin.o:	endwin.c
erase.o:	erase.c
erasechar.o:	erasechar.c
fixterm.o:	fixterm.c
flash.o:	flash.c
flushinp.o:	flushinp.c
getch.o:	getch.c
getstr.o:	getstr.c
idlok.o:	idlok.c
gettmode.o:	gettmode.c
has_ic.o:	has_ic.c
has_il.o:	has_il.c
idln.getst.o:	idln.getst.c
initkeypad.o:	initkeypad.c
initscr.o:	initscr.c
insch.o:	insch.c
insertln.o:	insertln.c
intrflush.o:	intrflush.c
keypad.o:	keypad.c
killchar.o:	killchar.c
leaveok.o:	leaveok.c
line_alloc.o:	line_alloc.c
ll_refresh.o:	ll_refresh.c
longname.o:	longname.c
m_addch.o:	m_addch.c
m_addstr.o:	m_addstr.c
m_clear.o:	m_clear.c
m_erase.o:	m_erase.c
m_move.o:	m_move.c
meta.o:	meta.c
m_refresh.o:	m_refresh.c
m_tstp.o:	m_tstp.c
makenew.o:	makenew.c
miniinit.o:	miniinit.c
move.o:	move.c
mvcur.o:	mvcur.c
mvprintw.o:	mvprintw.c
mvscanw.o:	mvscanw.c
mvwin.o:	mvwin.c
mvwprintw.o:	mvwprintw.c
mvwscanw.o:	mvwscanw.c
naps.o:	naps.c
newpad.o:	newpad.c
newterm.o:	newterm.c
newwin.o:	newwin.c
nl.o:	nl.c
nocbreak.o:	nocbreak.c
nocrmode.o:	nocrmode.c
nodelay.o:	nodelay.c
noecho.o:	noecho.c
nonl.o:	nonl.c
noraw.o:	noraw.c
nttychktrm.o:	nttychktrm.c
overlay.o:	overlay.c
overwrite.o:	overwrite.c
pnoutrfrsh.o:	pnoutrfrsh.c
prefresh.o:	prefresh.c
printw.o:	printw.c
putp.o:	putp.c
raw.o:	raw.c
reset_prog.o:	reset_prog.c
resetshell.o:	resetshell.c
resetterm.o:	resetterm.c
resetty.o:	resetty.c
restarttrm.o:	restarttrm.c
saveterm.o:	saveterm.c
savetty.o:	savetty.c
scanw.o:	scanw.c
scroll.o:	scroll.c
scrollok.o:	scrollok.c
set_term.o:	set_term.c
setbuffred.o:	setbuffred.c
setterm.o:	setterm.c
setupterm.o:	setupterm.c
showstring.o:	showstring.c
subwin.o:	subwin.c
tgetent.o:	tgetent.c
tgetflag.o:	tgetflag.c
tgetnum.o:	tgetnum.c
tgetstr.o:	tgetstr.c
tgoto.o:	tgoto.c
touchwin.o:	touchwin.c
tparm.o:	tparm.c
tputs.o:	tputs.c
traceonoff.o:	traceonoff.c
tstp.o:	tstp.c
two.twostr.o:	two.twostr.c
typeahead.o:	typeahead.c
unctrl.o:	unctrl.c
vidattr.o:	vidattr.c
vidputs.o:	vidputs.c
vsprintf.o:	vsprintf.c
vsscanf.o:	vsscanf.c
wattroff.o:	wattroff.c
wattron.o:	wattron.c
wattrset.o:	wattrset.c
wnoutrfrsh.o:	wnoutrfrsh.c
wprintw.o:	wprintw.c
wrefresh.o:	wrefresh.c
writechars.o:	writechars.c
wscanw.o:	wscanw.c
wstandend.o:	wstandend.c
wstandout.o:	wstandout.c

