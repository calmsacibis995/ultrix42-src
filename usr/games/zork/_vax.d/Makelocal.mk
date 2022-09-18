#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

AOUT=	zork

OBJS=	zork.o

zork.o:	zork.c

install:
	$(INSTALL) -c -s zork ${DESTROOT}/usr/games/zork
	$(CP) ../zorklib/dtext.dat ${DESTROOT}/usr/games/lib/dtext.dat
	$(CP) ../zorklib/dindex.dat ${DESTROOT}/usr/games/lib/dindex.dat
	$(CP) ../zorklib/doverlay ${DESTROOT}/usr/games/lib/doverlay
	$(CP) ../zorklib/dungeon ${DESTROOT}/usr/games/lib/dungeon

include $(GMAKERULES)
