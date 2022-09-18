#	@(#)Makelocal_vax.mk	4.1	(ULTRIX)	7/2/90
ASMSED=	../../include/asm.sed

$(OBJSMISC):
	cc -S $(CFLAGS) $(CINCLUDES) ../$*.c
	sed -f $(ASMSED) $*.s | as -o $*.o
	rm -f $*.s
