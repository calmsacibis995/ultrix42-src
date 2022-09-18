# @(#)Makelocal.mk	4.2	(ULTRIX)	12/6/90

include $(GMAKEVARS)

CFLAGS=
LDFLAGS=

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

OBJ=	dumpitime.o dumpmain.o dumpoptr.o dumptape.o dumptraverse.o \
	unctime.o statchk.o

ROBJ=	dumpitime.o dumprmain.o dumpoptr.o dumprmt.o dumprtape.o \
	dumptraverse.o unctime.o rstatchk.o

OBJS=	$(ROBJ) $(OBJ)

all:	rdump dump

rdump:	$(ROBJ)
	$(LDCMD) $(ROBJ)

dump:	$(OBJ)
	$(LDCMD) $(OBJ)

dumpitime.o:	dumpitime.c
dumpmain.o:	dumpmain.c
dumpoptr.o:	dumpoptr.c
dumptape.o:	dumptape.c
dumptraverse.o:	dumptraverse.c
unctime.o:	unctime.c
statchk.o:	statchk.c
dumprmt.o:	dumprmt.c

install:
	$(INSTALL) -c -s  dump $(DESTROOT)/bin
	$(RM) $(DESTROOT)/etc/dump
	$(LN) -s ../bin/dump $(DESTROOT)/etc/dump
	$(INSTALL) -c -s rdump $(DESTROOT)/bin
	$(RM) $(DESTROOT)/etc/rdump
	$(LN) -s ../bin/rdump $(DESTROOT)/etc/rdump

dumprmain.o: ../dumpmain.c
	$(CCCMD) -DRDUMP ../dumpmain.c
	$(MV) dumpmain.o dumprmain.o

rstatchk.o: statchk.c
	$(CCCMD) -DRDUMP ../statchk.c
	$(MV) statchk.o rstatchk.o

dumprtape.o: dumprtape.c
	$(CCCMD) -DRDUMP ../dumprtape.c

$(OBJ): dump.h 

include $(GMAKERULES)
