#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

DESTLIB=$(DESTROOT)/usr/games/lib/quiz.k
DESTLIST= $(DESTLIB)

AOUT=	quiz

OBJS=	quiz.o

quiz.o:	quiz.c

install:
	$(INSTALL) -c -s quiz $(DESTROOT)/usr/games/quiz
	$(CP) ../quiz.k/[a-z]* $(DESTLIB)

include $(GMAKERULES)
