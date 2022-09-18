#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	chess

OBJS=	chess.o

chess.o:	chess.c

install:
	$(INSTALL) -c chess $(DESTROOT)/usr/games/chess
	$(INSTALL) -c ../chess.bin $(DESTROOT)/usr/games/lib/chess

include $(GMAKERULES)
