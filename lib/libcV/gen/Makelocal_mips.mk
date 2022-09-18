#  @(#)Makelocal_mips.mk	4.1	ULTRIX	7/3/90

CFLAGS=	-O -Y

$(GENOBJS) getpw.o:
	$(CCCMD) $<
