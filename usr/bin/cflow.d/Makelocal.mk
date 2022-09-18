include $(GMAKEVARS)

include ../Makelocal_$(MACHINE).mk

CDEFINES = -DFLEXNAMES
CINCLUDES = -I$(LINT) -I$(MIP) -I. -I.. -I$(SRCROOT)/usr/include

AOUTS = dag nmf flip
ALL = $(AOUTS) lpfx

BIN = ${DESTROOT}/usr/bin
LIB = ${DESTROOT}/usr/lib

all:	$(ALL)

dag:	dag.o
nmf:	nmf.o
flip:	flip.o

dag.o:	dag.c
nmf.o:	nmf.c
flip.o:	flip.c

lpfx:	lpfx.o hash.o
	$(LDCMD) lpfx.o hash.o

lpfx.o:	lpfx.c $(LINT)/lmanifest $(MIP)/manifest
hash.o:	hash.c

install:
	$(INSTALL) -c -o root -g system -m 755 ../$(CFLOWSHELL) $(BIN)/cflow
	for i in ${ALL}; do \
	($(INSTALL) -c -s -o root -g system -m 755 $$i $(LIB)/$$i); done

include $(GMAKERULES)
