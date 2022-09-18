#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

CDEFINES=	-DBUG4 -DCXREF -DFLEXNAMES

AOUT=	xpass

OBJS=comm1.o optim.o pftn.o scan.o trees.o xdefs.o xlocal.o \
	lint.o hash.o cgram.o

$(OBJS):manifest macdefs mfile1

cgram.o:cgram.c
	$(CCCMD) cgram.c

cgram.c:cgram.y
	sed -e 's/\/\*CXREF\(.*\)\*\//\1/' ../cgram.y > gram.y
	yacc gram.y
	mv y.tab.c cgram.c
	-rm -f gram.y

comm1.o:comm1.c common
lint.o:lint.c lmanifest
hash.o:hash.c lint.c
optim.o:optim.c
pftn.o:pftn.c
scan.o:scan.c 
trees.o:trees.c
xdefs.o: xdefs.c
xlocal.o:xlocal.c lmanifest

install:
	$(INSTALL) -c -s xpass $(LIBDIR) $(DESTROOT)/usr/lib/xpass

include $(GMAKERULES)

