#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/bin $(DESTROOT)/bin

CDEFINES=-DBSD4X -Dunix

AOUT=	s5make

OBJS	= main.o doname.o misc.o files.o rules.o dosys.o gram.o \
	  dyndep.o prtmem.o sgetl.o

$(OBJS):  defs

main.o:		main.c
doname.o:	doname.c
misc.o:		misc.c
files.o:	files.c
rules.o:	rules.c
dosys.o:	dosys.c
gram.o:		gram.y
dyndep.o:	dyndep.c
prtmem.o:	prtmem.c
sgetl.o:	sgetl.c

install:
	$(INSTALL) -c -m 755 -s s5make $(DESTROOT)/usr/bin/s5make
	$(RM) $(DESTROOT)/bin/s5make
	$(LN) -s ../usr/bin/s5make $(DESTROOT)/bin/s5make
	$(RM) $(DESTROOT)/usr/bin/make
	$(LN) -s s5make $(DESTROOT)/usr/bin/make
	$(RM) $(DESTROOT)/bin/make
	$(LN) -s ../usr/bin/s5make $(DESTROOT)/bin/make

include $(GMAKERULES)
