#@(#)Makefile	4.1	Ultrix	7/17/90
ALL=	strings bitops float forloop lib lib-s array nocount pack mark process\
	mem mems
all:	$(ALL)
strings:	strings.mod
	mod -o strings -C -g strings.mod
	@echo strings will complain if errors in string module
	strings
bitops:	bitops.mod
	mod -o bitops -C -g bitops.mod
	@echo bitops will complain if errors in bitoperations implementation
	bitops
float:	float.mod
	mod -o float -C -g float.mod
	@echo float will complain if errors in floating conversion
	float
forloop:	forloop.mod
	mod -o forloop -C -g forloop.mod
	@echo forloop will complain if errors in char for loop
	forloop
lib:	lib.mod
	@echo lib should generate no compile errors
	mod -o lib -C -g lib.mod
lib-s:	lib-s.mod
	@echo lib-s should generate no compile errors
	mod -o lib-s -s -C -g lib-s.mod
array:	array.mod
	@echo 'compiler should detect 4 "open array for parameters only"'
	@echo 'errors, 1 "@nocount for Modula-2 parameter" error, and 1'
	@echo 'cardinal division overflow'
	-mod -o array -C -g array.mod
	touch array
nocount:	nocount.mod
	@echo nocount should generate no compile errors
	@echo look at code to see if right thing is happening
	mod -S -g nocount.mod
	touch nocount
pack:	pack.mod
	@echo pack should generate no compile errors
	@echo look at code to see if right thing is happening
	mod -o pack -C -g pack.mod
mark:	mark.mod
	@echo mark should generate no compile errors
	mod -o mark -C -g mark.mod
process:	process.mod
	@echo process should run and generate a bunch of output
	mod -o process -C -g process.mod
	process
mem:	mem.mod
	@echo mem should run and generate a bunch of output
	mod -o mem -C -g mem.mod
	mem
mems:	mems.mod
	@echo mem should run and generate a bunch of output
	mod -o mem -C -g mems.mod
	mem
clean:
	-rm -f $(ALL)
