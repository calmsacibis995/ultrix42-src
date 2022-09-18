# @(#)Makelocal.mk	4.1	(ULTRIX)	7/3/90

include $(GMAKEVARS)

#	path to the directory containing as
AS =	../../as

DFLAGS = -DADB
CFLAGS=	-O -w $(DFLAGS)
CINCLUDES= -I. -I.. -I$(AS) -I$(SRCROOT)/usr/include

AOUT=	adb

OBJS=	access.o command.o expr.o format.o input.o main.o message.o \
	opset.o optab.o output.o pcs.o print.o runpcs.o setup.o sym.o

access.o:	access.c defs.h
command.o:	command.c defs.h
expr.o:		expr.c defs.h
format.o:	format.c defs.h
input.o:	input.c defs.h
main.o:		main.c defs.h
opset.o:	opset.c defs.h
output.o:	output.c
pcs.o:		pcs.c defs.h
print.o:	print.c defs.h
runpcs.o:	runpcs.c defs.h
setup.o:	setup.c defs.h
sym.o:		sym.c defs.h

message.o:	message.c mac.h mode.h
	$(CCCMD) -R ../$<

optab.o:	optab.c defs.h instrs.adb
	$(CCCMD) -R ../$<

instrs.adb: $(AS)/instrs
	(echo FLAVOR ADB; cat $(AS)/instrs) \
		| awk -f $(AS)/instrs > instrs.adb

install:
	$(INSTALL) -c -s adb $(DESTROOT)/bin

include $(GMAKERULES)
