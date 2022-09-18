#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

M=../../mip
CDEFINES=-DFLEXNAMES
CINCLUDES=-I$M -I.. -I$(SRCROOT)/usr/include
LIBDIR=../../../../usr/bin/lint.d

LINTLIBS=llib-port.ln llib-lc.ln llib-lcP.ln llib-lcV.ln \
	llib-lm.ln llib-lmV.ln llib-lmp.ln llib-lcurses.ln

CFILES=	trees.c optim.c pftn.c scan.c xdefs.c comm1.c 
OBJS=	xdefs.o scan.o comm1.o pftn.o trees.o optim.o

all:	lpass1 lpass2 $(LINTLIBS)

lpass1: $(OBJS) cgram.o hash.o lint.o
	$(CC) $(OBJS) cgram.o hash.o lint.o -o lpass1

$(CFILES):
	$(RM) $@
	$(LN) -s $M/$@ $@

trees.c:	$M/trees.c
optim.c:	$M/optim.c
pftn.c:		$M/pftn.c
scan.c:		$M/scan.c
xdefs.c:	$M/xdefs.c
comm1.c:	$M/comm1.c

$(OBJS):	$M/manifest $M/mfile1 macdefs
	$(CC) -c $(CFLAGS) $(CDEFINES) $(CINCLUDES) $<

hash.o:		hash.c
lint.o:		lint.c

trees.o:	trees.c
optim.o:	optim.c
pftn.o:		pftn.c
scan.o:		scan.c
xdefs.o:	xdefs.c
comm1.o:	$M/common comm1.c

cgram.o:	$M/cgram.y
	$(YACC) $M/cgram.y
	$(MV) y.tab.c cgram.c
	$(CC) -c $(CFLAGS) $(CDEFINES) $(CINCLUDES) cgram.c

llib-port.ln: lpass1
	$(RM) llib-port
	$(LN) -s $(LIBDIR)/llib-port .
	-($(CPP) -C -Dlint llib-port | ./lpass1 -puv > llib-port.ln )

llib-lm.ln: lpass1
	$(RM) llib-lm
	$(LN) -s $(LIBDIR)/llib-lm .
	-($(CPP) -C -Dlint llib-lm | ./lpass1 -puv > llib-lm.ln )

llib-lmV.ln: lpass1
	-($(CPP) -C -Dlint -DSYSTEM_FIVE llib-lm | ./lpass1 -puv > llib-lmV.ln )

llib-lmp.ln: lpass1
	$(RM) llib-lmp
	$(LN) -s $(LIBDIR)/llib-lmp .
	-($(CPP) -C -Dlint llib-lmp | ./lpass1 -puv > llib-lmp.ln )

llib-lc.ln: lpass1
	$(RM) llib-lc
	$(LN) -s $(LIBDIR)/llib-lc .
	-($(CPP) -C -Dlint llib-lc | ./lpass1 -v > llib-lc.ln )

llib-lcP.ln: lpass1
	-($(CPP) -C -Dlint -DPOSIX llib-lc | ./lpass1 -v > llib-lcP.ln )

llib-lcV.ln: lpass1
	-($(CPP) -C -Dlint -DSYSTEM_FIVE llib-lc | ./lpass1 -v > llib-lcV.ln )

llib-lcurses.ln: lpass1
	$(RM) llib-lcurses
	$(LN) -s $(LIBDIR)/llib-lcurses .
	-($(CPP) -C -Dlint llib-lcurses | ./lpass1 -v > llib-lcurses.ln )

lpass2: lpass2.o hash.o
	$(CC) lpass2.o hash.o -o lpass2

lpass2.o: lpass2.c $M/manifest lmanifest

install:
	@-if [ ! -d $(DESTROOT)/usr/lib/lint ] ; then \
		$(MKDIR) $(DESTROOT)/usr/lib/lint ; \
		$(CHOWN) root $(DESTROOT)/usr/lib/lint ; \
		$(CHGRP) system $(DESTROOT)/usr/lib/lint ; \
		$(CHMOD) 0755 $(DESTROOT)/usr/lib/lint ; \
	fi
	$(INSTALL) -c -s lpass1 $(DESTROOT)/usr/lib/lint/lint1
	$(INSTALL) -c -s lpass2 $(DESTROOT)/usr/lib/lint/lint2
	@for i in $(LIBDIR)/llib-*; do \
		$(ECHO) "$(INSTALL) -c -m 644 $$i $(DESTROOT)/usr/lib/lint/`basename $$i`"; \
		$(INSTALL) -c -m 644 $$i $(DESTROOT)/usr/lib/lint/`basename $$i`; \
	done
	@for i in llib-*; do \
		$(ECHO) "$(INSTALL) -c -m 644 $$i $(DESTROOT)/usr/lib/lint/$$i"; \
		$(INSTALL) -c -m 644 $$i $(DESTROOT)/usr/lib/lint/$$i; \
	done
	$(INSTALL) -c ../SHELL $(DESTROOT)/usr/bin/lint

include $(GMAKERULES)
