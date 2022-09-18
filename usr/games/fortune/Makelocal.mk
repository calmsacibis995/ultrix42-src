#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

FORTUNES=	scene 
SOURCE=		fortune.c strfile.h strfile.c unstr.c $(FORTUNES)
LIBDIR=		$(DESTROOT)/usr/games/lib
BINDIR=		$(DESTROOT)/usr/games
OWN=		daemon

DESTLIST=$(LIBDIR)

all: fortune strfile unstr fortunes.dat

fortune: strfile.h fortune.c
	$(CC) $(CFLAGS) -DFORTFILE='"$(LIBDIR)/fortunes.dat"' -o fortune ../fortune.c

strfile: strfile.h strfile.c
	$(CC) $(CFLAGS) -o strfile ../strfile.c

unstr: strfile.h unstr.c
	$(CC) $(CFLAGS) -o unstr ../unstr.c

fortunes.dat: fortunes strfile
	./strfile fortunes

fortunes:
	cat ../scene > ./fortunes
	echo "%%" >> ./fortunes

install:
	install -c -m 600 -o $(OWN) fortunes.dat $(LIBDIR)/fortunes.dat
	install -c -s -m 700 -o $(OWN) strfile $(LIBDIR)/strfile
	install -c -s -m 4711 -o $(OWN) fortune $(BINDIR)/fortune

include $(GMAKERULES)
