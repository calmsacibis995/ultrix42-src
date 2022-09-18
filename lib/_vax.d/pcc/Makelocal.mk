# @(#)Makelocal.mk	4.2	(ULTRIX)	8/13/90
include $(GMAKEVARS)

M=../../mip

CDEFINES = -DASSTRINGS -DSTABDOT -DLCOMM -DBUFSTDERR -DFLEXNAMES \
    "-DFIXSTRUCT=outstruct" -DLONGFIELDS
CINCLUDES = -I.. -I${M} -I$(SRCROOT)/usr/include

#next 4 lines were in pre3.2 makefile (stuarth 2/2/89)
# all: comp fort	not making f1 until f77 copy of sources is merged back
#all: getmipsrc comp	getmipsrc refers to a makefile in mip which I don't have
#getmipsrc:
#	(cd $M; make sources)

all: comp

comp: rodata.o cgram.o xdefs.o scan.o pftn.o trees.o optim.o code.o local.o \
		reader.o local2.o order.o match.o allo.o comm1.o table.o stab.o
	$(LDCMD) -z rodata.o cgram.o xdefs.o scan.o pftn.o trees.o \
		optim.o code.o local.o reader.o local2.o order.o match.o \
		allo.o comm1.o table.o stab.o

trees.o: $M/manifest ../macdefs $M/mfile1 $M/trees.c
	$(CCCMD) $M/trees.c
optim.o: $M/manifest ../macdefs $M/mfile1 $M/optim.c
	$(CCCMD) $M/optim.c
pftn.o: $M/manifest ../macdefs $M/mfile1 $M/pftn.c
	$(CCCMD) $M/pftn.c
scan.o: $M/manifest ../macdefs $M/mfile1 $M/scan.c
	$(CCCMD) $M/scan.c
xdefs.o: $M/manifest $M/mfile1 ../macdefs $M/xdefs.c
	$(CCCMD) $M/xdefs.c
reader.o: $M/manifest $M/mfile2 ../mac2defs ../macdefs $M/reader.c
	$(CCCMD) $M/reader.c
match.o: $M/manifest $M/mfile2 ../mac2defs ../macdefs $M/match.c
	$(CCCMD) $M/match.c
allo.o: $M/manifest $M/mfile2 ../mac2defs ../macdefs $M/allo.c
	$(CCCMD) $M/allo.c
comm1.o: $M/manifest $M/mfile1 $M/common ../macdefs $M/comm1.c
	$(CCCMD) $M/comm1.c

code.o: $M/manifest ../macdefs $M/mfile1 ../code.c
	$(CCCMD) ../code.c
local.o: $M/manifest ../macdefs $M/mfile1 ../local.c
	$(CCCMD) ../local.c
local2.o: $M/manifest $M/mfile2 ../mac2defs ../macdefs ../local2.c
	$(CCCMD) ../local2.c
order.o: $M/manifest $M/mfile2 ../mac2defs ../macdefs ../order.c
	$(CCCMD) ../order.c
stab.o: $M/manifest $M/mfile1 ../mac2defs ../macdefs ../stab.c
	$(CCCMD) ../stab.c

rodata.o: rodata.c
	$(CCCMD) -R rodata.c
rodata.c cgram.c: $M/cgram.y yyfix
	$(YACC) $M/cgram.y
	rm -f rodata.c
	sh yyfix yyexca yyact yypact yypgo yyr1 yyr2 yychk yydef
	mv y.tab.c cgram.c
cgram.o: $M/manifest $M/mfile1 ../macdefs cgram.c
	$(CCCMD) cgram.c

table.o: $M/manifest $M/mfile2 ../mac2defs ../macdefs ../table.c
	$(CCCMD) -R ../table.c

yyfix: 
	(cp ../:$@ $@; chmod 555 $@)

lintall:
	lint -hpv $(CINCLUDES)  $M/cgram.c $M/xdefs.c $M/scan.c $M/pftn.c \
		$M/trees.c $M/optim.c ../code.c ../local.c $M/reader \
		../local2.c ../order.c $M/match.c $M/allo.c $M/comm1.c ../table.c

fort: comp fort.o freader.o fallo.o fmatch.o ftable.o forder.o flocal2.o \
	fcomm2.o
	@echo "f1 is usually made from sources in /usr/src/usr.lib/f77 ..."
	$(LDCMD) -z fort.o freader.o fallo.o fmatch.o ftable.o \
		forder.o flocal2.o fcomm2.o
fort.o: fort.h $M/fort.c
	$(CCCMD) $M/fort.c
freader.o: reader.o
	$(CCCMD) $M/freader.c
fallo.o: allo.o
	$(CCCMD) $M/fallo.c
fmatch.o: match.o
	$(CCCMD) $M/fmatch.c
ftable.o: table.o
	$(CCCMD) $M/ftable.c
forder.o: order.o
	$(CCCMD) $M/forder.c
flocal2.o: local2.o
	$(CCCMD) $M/flocal2.c
fcomm2.o: $M/common
	$(CCCMD) $M/fcomm2.c
fort.o freader.o fallo.o fmatch.o ftable.o forder.o flocal2.o fcomm2.o: \
	$M/mfile2 $M/manifest ../macdefs ../mac2defs

pretools tools1 tools2:	all

pretools tools1 tools2 install:
	rm -f ccom
	ln comp ccom
	install -c -m 755 ccom ${DESTROOT}/usr/lib
#	rm -f ${DESTROOT}/usr/lib/f1
#	cp fort ${DESTROOT}/usr/lib/f1
include $(GMAKERULES)
