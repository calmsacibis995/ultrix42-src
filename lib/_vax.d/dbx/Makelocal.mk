#  @(#)Makelocal.mk	4.2  ULTRIX  11/9/90

include $(GMAKEVARS)

.SUFFIXES:
.SUFFIXES: .h .y .c .s .o

YFLAGS=	-d

OBJS=	Xttylib.o \
	commands.o \
	asm.o \
	events.o \
	c.o \
	cerror.o \
	check.o \
	coredump.o \
	debug.o \
	eval.o \
	fortran.o \
	history.o\
	keywords.o \
	languages.o \
	library.o \
	lists.o \
	machine.o \
	main.o \
	mappings.o \
	modula-2.o \
	names.o \
	object.o \
	operators.o \
	pascal.o \
	printsym.o \
	process.o \
	runtime.o \
	scanner.o \
	source.o \
	stabstring.o \
	symbols.o \
	tree.o \
	ops.o

HFILES = \
	asm.h \
	events.h \
	c.h \
	check.h \
	coredump.h \
	debug.h \
	eval.h \
	fortran.h \
	history.h\
	keywords.h \
	languages.h \
	lists.h \
	machine.h \
	main.h \
	mappings.h \
	modula-2.h \
	names.h \
	object.h \
	operators.h \
	pascal.h \
	printsym.h \
	process.h \
	runtime.h \
	scanner.h \
	source.h \
	stabstring.h \
	symbols.h \
	tree.h \
	ops.h

all:	dbx

dbx:	makedefs mkdate $(HFILES) $(OBJS)
	rm -f date.c
	./mkdate > date.c
	$(CC) $(LDFLAGS) date.c $(OBJS) $(LIBRARIES) -o dbx

commands.o: commands.y

asm.o:			asm.c
events.o:		events.c
c.o:			c.c
cerror.o:		cerror.s
check.o:		check.c
coredump.o:		coredump.c
debug.o:		debug.c
eval.o:			eval.c
fortran.o:		fortran.c
history.o:		history.c
keywords.o:		keywords.c
languages.o:		languages.c
library.o:		library.c
lists.o:		lists.c
machine.o:		machine.c
main.o:			main.c
mappings.o:		mappings.c
modula-2.o:		modula-2.c
names.o:		names.c
object.o:		object.c
operators.o:		operators.c
pascal.o:		pascal.c
printsym.o:		printsym.c
process.o:		process.c
runtime.o:		runtime.c
scanner.o:		scanner.c
source.o:		source.c
stabstring.o:		stabstring.c
symbols.o:		symbols.c
tree.o:			tree.c
ops.o:			ops.c
Xttylib.o:		Xttylib.c

asm.h:			asm.c
events.h:		events.c
c.h:			c.c
check.h:		check.c
coredump.h:		coredump.c
debug.h:		debug.c
eval.h:			eval.c
fortran.h:		fortran.c
keywords.h:		keywords.c
languages.h:		languages.c
lists.h:		lists.c
machine.h:		machine.c
main.h:			main.c
mappings.h:		mappings.c
modula-2.h:		modula-2.c
names.h:		names.c
object.h:		object.c
operators.h:		operators.c
pascal.h:		pascal.c
printsym.h:		printsym.c
process.h:		process.c
runtime.h:		runtime.c
scanner.h:		scanner.c
source.h:		source.c
stabstring.h:		stabstring.c
symbols.h:		symbols.c
tree.h:			tree.c
ops.h:			ops.c

.c.h:
	$(LN) ../$*.c $*.c
	./makedefs -f $*.c $*.h
	$(RM) $*.c

makedefs: makedefs.c library.o cerror.o
	$(CC) -g ../makedefs.c library.o cerror.o -o makedefs

mkdate: mkdate.c
	$(CC) -g ../mkdate.c -o mkdate

install: dbx
	$(INSTALL) -c -s dbx $(DESTROOT)/usr/ucb/dbx

include $(GMAKERULES)
