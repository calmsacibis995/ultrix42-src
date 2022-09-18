#
#	@(#)Makefile	1.8	(ULTRIX)	2/11/86
#
# Makefile for PCC
#
# These symbols are used to configure the compiler:
#	ASSTRINGS	assembler handles string initializations
#	STABDOT		assembler understands .stabd
#	LCOMM		assembler supports .lcomm
#	FIXSTRUCT	no trickery (just output structure)
#
#	FORT		get f77-style pass 2
#

CONFIG=	-DASSTRINGS -DSTABDOT -DLCOMM "-DFIXSTRUCT=outstruct"
O = -O
CFLAGS = $(O) -I. -I$(M) $(CONFIG)

FOPTS = -DFORT $(CFLAGS)
LDFLAGS =

RM=	/bin/rm -f

FFILES=	freader.c fallo.c fmatch.c ftable.c forder.c flocal2.c fcomm2.c
FOBJS=	freader.o fallo.o fmatch.o ftable.o forder.o flocal2.o fcomm2.o

P2INCS=	$M/pass2.h macdefs.h mac2defs.h $M/manifest.h pcclocal.h $M/config.h \
	$M/ndu.h /usr/include/stdio.h
P2ONEP=	$(P2INCS) $M/onepass.h

M=../mip

MAKEFILE = Makefile

LINKER = cc

PRINT = pr

PROGRAM = f1

#
# 'f1' is the f77 and pc code generator.
#
all: f1

f1 : fort.o ${FOBJS}
	${CC} ${LDFLAGS} fort.o ${FOBJS} -o f1

fort.o: $(P2INCS) fort.h $M/fort.c
	$(CC) -c $(FOPTS) $M/fort.c

freader.o: $(P2INCS) $M/reader.c
	ln $M/reader.c freader.c
	$(CC) -c $(FOPTS) freader.c
	$(RM) freader.c
fallo.o: $(P2INCS) $M/allo.c
	ln $M/allo.c fallo.c
	$(CC) -c $(FOPTS) fallo.c
	$(RM) fallo.c
fmatch.o: $(P2INCS) $M/match.c
	ln $M/match.c fmatch.c
	$(CC) -c $(FOPTS) fmatch.c
	$(RM) fmatch.c
ftable.o: $(P2INCS) table.c
	ln table.c ftable.c
	$(CC) -c -R $(FOPTS) ftable.c
	$(RM) ftable.c
forder.o: $(P2INCS) order.c
	ln order.c forder.c
	$(CC) -c $(FOPTS) forder.c
	$(RM) forder.c
flocal2.o: $(P2INCS) local2.c
	ln local2.c flocal2.c
	$(CC) -c $(FOPTS) flocal2.c
	$(RM) flocal2.c
fcomm2.o: $(P2INCS) $M/common.c
	ln $M/common.c fcomm2.c
	$(CC) -c $(FOPTS) -DPASS2COMMON fcomm2.c
	$(RM) fcomm2.c

install: all
	install -c -s f1 ${DESTROOT}/lib/f1

GREP=	egrep

pcclocal.h: localdefs.h ../../include/pcc.h
	$(RM) pcclocal.h
	cat ../../include/pcc.h localdefs.h | $(GREP) '^#[ 	]*(define[ 	][ 	]*PCC(F|T|TM|OM)?_|ifdef|ifndef|endif)' | sed -e 's/PCC[A-Z]*_//' > pcclocal.h 

pcctokens: localdefs.h ../../include/pcc.h
	$(RM) pcctokens
	cat ../../include/pcc.h localdefs.h | $(GREP) '^#[ 	]*define[ 	][ 	]*PCC_' | sed -e 's/^#[ 	]*define[ 	][ 	]*PCC_/%term	/' > pcctokens

clean:
	$(RM) *.o f1 cgram.c rodata.c pcctokens pcclocal.h gram.in 
	cd ../mip; rm -f *.o

clobber: clean
	rm -f *.[ch] gram.* core Makefile
	cd ../mip; rm -f *.[ch] core
sccsinfo:
	sccs info
	cd ../mip; sccs info
sccsget:
	sccs get SCCS
	cd ../mip; sccs get SCCS

depend :;	@echo Construnct dependencies manually
