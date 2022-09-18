# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90
#
include $(GMAKEVARS)

CFLAGS=-O
CINCLUDES=-I. -I..
YFLAGS=-d
0FILES.c = 0.alloc.c 0.args.c 0.def.c 0.extr.c 0.graph.c 0.list.c 0.parts.c 0.string.c
0FILES.o = 0.alloc.o 0.args.o 0.def.o 0.extr.o 0.graph.o 0.list.o 0.parts.o 0.string.o

1FILES.c = 1.finish.c 1.form.c 1.fort.c 1.hash.c 1.init.c 1.line.c 1.main.c 1.node.c 1.recog.c 1.tables.c
1FILES.o = 1.finish.o 1.form.o 1.fort.o 1.hash.o 1.init.o 1.line.o 1.main.o 1.node.o 1.recog.o 1.tables.o

2FILES.c = 2.dfs.c 2.dom.c 2.head.c 2.inarc.c 2.main.c 2.tree.c
2FILES.o = 2.dfs.o 2.dom.o 2.head.o 2.inarc.o 2.main.o 2.tree.o

3FILES.c = 3.branch.c 3.flow.c 3.loop.c 3.main.c 3.reach.c 3.then.c
3FILES.o = 3.branch.o 3.flow.o 3.loop.o 3.main.o 3.reach.o 3.then.o

4FILES.c = 4.brace.c 4.form.c 4.main.c 4.out.c
4FILES.o = 4.brace.o 4.form.o 4.main.o 4.out.o

HDRS = 1.defs.h 1.incl.h 2.def.h 3.def.h 4.def.h b.h def.h
MSRCS = bdef.c main.c tree.c Makefile beauty.y lextab.l

SRCS = $(0FILES.c) $(1FILES.c) $(2FILES.c) \
	$(3FILES.c) $(4FILES.c) $(HDRS) $(MSRCS)

OBJS= $(0FILES.o) $(1FILES.o) $(2FILES.o) \
	$(3FILES.o) $(4FILES.o) main.o
AOUT=structure

all:	structure beautify

install:
	-if [ ! -d ${DESTROOT}/usr/lib/struct ]; then \
		mkdir ${DESTROOT}/usr/lib/struct; \
	else true; \
	fi
	install -c -s structure $(DESTROOT)/usr/lib/struct/structure
	install -c -s beautify $(DESTROOT)/usr/lib/struct/beautify
	install -c ../struct $(DESTROOT)/usr/bin/struct

beautify:	beauty.o tree.o lextab.o bdef.o
	$(LDCMD) -O beauty.o tree.o lextab.o bdef.o -lln

tree.o bdef.o:
	$(CCCMD) ../$*.c

main.o $(0FILES.o) $(1FILES.o) $(2FILES.o) $(3FILES.o) $(4FILES.o): def.h
$(1FILES.o): 1.defs.h 1.incl.h
$(2FILES.o): 2.def.h
$(3FILES.o): 3.def.h
$(4FILES.o): 4.def.h

y.tab.h: beauty.y beauty.c
lextab.o tree.o: y.tab.h
lextab.o tree.o: b.h

beauty.o:	beauty.c
	$(CCCMD) $*.c

beauty.c:	beauty.y
	$(YACC) $(YFLAGS) ../beauty.y
	mv y.tab.c beauty.c

lextab.o:	../lextab.l
	$(LEX) $(LFLAGS) ../lextab.l
	$(CCCMD) lex.yy.c
	mv lex.yy.o lextab.o

0.alloc.o: 0.alloc.c
0.args.o: 0.args.c
0.def.o: 0.def.c
0.extr.o: 0.extr.c
0.graph.o: 0.graph.c
0.list.o: 0.list.c
0.parts.o: 0.parts.c
0.string.o: 0.string.c
1.finish.o: 1.finish.c
1.form.o: 1.form.c
1.fort.o: 1.fort.c
1.hash.o: 1.hash.c
1.init.o: 1.init.c
1.line.o: 1.line.c
1.main.o: 1.main.c
1.node.o: 1.node.c
1.recog.o: 1.recog.c
1.tables.o: 1.tables.c
2.dfs.o: 2.dfs.c
2.dom.o: 2.dom.c
2.head.o: 2.head.c
2.inarc.o: 2.inarc.c
2.main.o: 2.main.c
2.tree.o: 2.tree.c
3.branch.o: 3.branch.c
3.flow.o: 3.flow.c
3.loop.o: 3.loop.c
3.main.o: 3.main.c
3.reach.o: 3.reach.c
3.then.o: 3.then.c
4.brace.o: 4.brace.c
4.form.o: 4.form.c
4.main.o: 4.main.c
4.out.o: 4.out.c
bdef.o: bdef.c
main.o: main.c
tree.o: tree.c

include $(GMAKERULES)
