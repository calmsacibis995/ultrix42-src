#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUTS= boggle comp
DESTLIST= ${DESTROOT}/usr/games

all: bogdict
bogdict: comp ../sfile ../w2a
	sed -f ../sfile ../w2a | uniq | ./comp >bogdict

boggle:		boggle.o
boggle.o:	boggle.c

comp:		comp.o
comp.o:		comp.c

install:
	$(INSTALL) -c -s boggle ${DESTROOT}/usr/games/boggle
	$(INSTALL) -c bogdict ${DESTROOT}/usr/games/bogdict

include $(GMAKERULES)
