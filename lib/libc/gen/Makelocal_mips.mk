#  @(#)Makelocal_mips.mk	4.1  ULTRIX  7/3/90

all:	$(STDOBJS) $(VOBJS) $(FPOBJS) errlst.o atoi.o atol.o

errlst.o: errlst.c
	cc -c -O ../errlst.c

atoi.o atol.o:	
	$(CC) -c $(CDEBUG) $(CDEFINES) $(CINCLUDES) -G 0 ../$(@:.o=.c)
	$(MV) $@ G0/$@
	$(CC) -c $(CDEBUG) $(CDEFINES) $(CINCLUDES) ../$(@:.o=.c)

$(STDOBJS) $(VOBJS) $(FPOBJS):
	$(CCCMD) -G 0 ../$<
	$(MV) $*.o G0/$*.o
	$(CCCMD) ../$<

clean: cleanG0

cleanG0:
	-$(RM) G0/*
