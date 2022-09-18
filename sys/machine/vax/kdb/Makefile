# @(#)Makefile	4.1	ULTRIX	7/2/90
#
#	Makefile	4.3	82/10/27
#
DESTROOT= 
DFLAGS = -Dvax -DADB -DDEBUG -DKDEBUG -DKERNEL
CFLAGS=	-O -w ${DFLAGS} 
CFILES= access.c command.c expr.c format.c input.c main.c message.c \
	opset.c optab.c pcs.c print.c runpcs.c setup.c sym.c \
	kdb_ptrace.c kdb_misc.c kdb_mtpr.c 
OFILES= access.o command.o expr.o format.o input.o main.o message.o \
	opset.o optab.o pcs.o print.o runpcs.o setup.o sym.o \
	kdb_ptrace.o kdb_misc.o kdb_mtpr.o

#
#	path to the directory containing as
#
ASPATH = .
AS =	as
C2 =	/lib/c2
kdb.o:	${OFILES} 
	ld -r -o kdb.o ${OFILES} 

kdbload: kdbload.c kdb.h
	$(CC) -o kdbload -g kdbload.c

kdbenter:
	$(CC) -E kdbenter_syscall.c | $(AS) -o kdbenter_syscall.o
	$(CC) kdbenter.c kdbenter_syscall.o -o kdbenter

#defs.h:	mac.h mode.h machine.h

access.o:	mac.h mode.h machine.h defs.h access.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  access.c
command.o:	mac.h mode.h machine.h defs.h command.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  command.c
expr.o:		mac.h mode.h machine.h defs.h expr.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  expr.c
format.o:	mac.h mode.h machine.h defs.h format.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  format.c
input.o:	mac.h mode.h machine.h defs.h input.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  input.c
main.o:		mac.h mode.h machine.h defs.h main.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  main.c
message.o:	mac.h mode.h message.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c -R message.c
pcs.o:		mac.h mode.h machine.h defs.h pcs.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  pcs.c
print.o:	mac.h mode.h machine.h defs.h print.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  print.c
setup.o:	mac.h mode.h machine.h defs.h kdb.h setup.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  setup.c
sym.o:		mac.h mode.h machine.h defs.h sym.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  sym.c
kdb_misc.o:	mac.h kdb_misc.c
	${CC} ${CFLAGS} -I../../../VAX/BINARY -I../../ -c  kdb_misc.c


runpcs.o:	mac.h mode.h machine.h defs.h runpcs.c
	${CC} ${CFLAGS} -S -I../../../VAX/BINARY -I../../ runpcs.c
	${C2} runpcs.s | ../inline/inline | ${AS} -o runpcs.o
	rm runpcs.s

kdb_ptrace.o:	kdb_ptrace.c
	${CC} ${CFLAGS} -S -I../../../VAX/BINARY -I../../   kdb_ptrace.c
	${C2} kdb_ptrace.s | ../inline/inline | ${AS} -o kdb_ptrace.o
	rm kdb_ptrace.s

kdb_mtpr.o:	kdb_mtpr.c
	${CC} ${CFLAGS} -S -I../../../VAX/BINARY -I../../   kdb_mtpr.c
	${C2} kdb_mtpr.s | ../inline/inline | ${AS} -o kdb_mtpr.o
	rm kdb_mtpr.s

opset.o:	mac.h mode.h machine.h defs.h opset.c
	${CC} ${CFLAGS} -c -I../../../VAX/BINARY -I../../  -I$(ASPATH) opset.c

optab.o:	mac.h mode.h machine.h defs.h optab.c instrs.adb
	${CC} ${CFLAGS} -R -c -I../../../VAX/BINARY -I../../  -I$(ASPATH) optab.c

instrs.adb: $(ASPATH)/instrs
	(echo FLAVOR ADB; cat $(ASPATH)/instrs) | awk -f $(ASPATH)/instrs > instrs.adb

install: adb
	install -c -s adb ${DESTROOT}/bin

clean:
	rm -f adb ${OFILES}
	rm -f a.out core *.s errs kdbload kdb.o kdbenter.o kdbenter_syscall.o
	rm -f kdbenter instrs.adb 

clobber: clean
	rm -f *.[hclyp] Makefile instrs instrs.adb

sccsinfo:
	sccs info

sccsget:
	sccs get SCCS

print:
	@ls -l | pr
	@pr -f Makefile *.h ${CFILES}
