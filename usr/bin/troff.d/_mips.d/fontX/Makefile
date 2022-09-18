#
#	@(#)Makefile	4.1 ULTRIX	7/17/90
#
mkfont: mkfont.o
	cc mkfont.o -o mkfont

clean:
	rm -f *.o a.out errs

clobber: clean
	rm -f *.n linkrc Makefile

sccsinfo:
	sccs info

sccsget:
	sccs get SCCS

.DEFAULT:
	sccs get $<
