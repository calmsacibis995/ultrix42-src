# @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

COBJS=	restore.o dirs.o symtab.o utilities.o

OBJS=	$(COBJS) main.o tape.o statchk.o

ROBJS=  $(COBJS) rmain.o rtape.o dumprmt.o rstatchk.o	

CINCLUDES=-I. -I.. -I../../dump.d  -I$(SRCROOT)/usr/include

all: rrestore restore

restore: $(OBJS)
	$(LDCMD) $(OBJS)

rrestore: $(ROBJS)
	$(LDCMD) $(ROBJS)

rmain.o: main.c
	$(CCCMD) -DRRESTORE ../main.c
	$(MV) main.o rmain.o

statchk.o: ../../dump.d/statchk.c dump.h
	$(CCCMD) ../../dump.d/statchk.c

rstatchk.o: ../../dump.d/statchk.c
	$(CCCMD) -DRRESTORE ../../dump.d/statchk.c
	$(MV) statchk.o rstatchk.o

rtape.o: tape.c
	$(CCCMD) -DRRESTORE ../tape.c
	$(MV) tape.o rtape.o

dumprmt.o: ../../dump.d/dumprmt.c dump.h
	$(CCCMD) ../../dump.d/dumprmt.c

restore.o:	restore.c dump.h
dirs.o:		dirs.c dump.h
symtab.o:	symtab.c dump.h
utilities.o:	utilities.c dump.h
main.o:		main.c dump.h
tape.o:		tape.c dump.h

dump.h:		../../dump.d/dump.h

install:
	$(INSTALL) -c -s restore $(DESTROOT)/bin/restore
	$(RM) $(DESTROOT)/etc/restore
	$(LN) -s ../bin/restore $(DESTROOT)/etc/restore
	$(INSTALL) -c -s -m 4755 rrestore $(DESTROOT)/bin/rrestore
	$(RM) $(DESTROOT)/etc/rrestore
	$(LN) -s ../bin/rrestore $(DESTROOT)/etc/rrestore


include $(GMAKERULES)
