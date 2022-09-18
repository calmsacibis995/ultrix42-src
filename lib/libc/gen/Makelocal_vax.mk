#  @(#)Makelocal_vax.mk	4.1	ULTRIX  7/3/90

LD=ld

VAXOBJS=nlist.o

all:	$(STDOBJS) $(VOBJS) $(FPOBJS) $(VAXOBJS) errlst.o atoi.o atol.o

nlist.o:	nlist.c
errlst.o:	errlst.c

errlst.o:	errlst.c
	cc -S ../errlst.c
	ed - <../:errfix errlst.s
	as -o errlst.o errlst.s
	cp errlst.o profiled/.
	cp errlst.o gfloat/.
	rm -f  errlst.[sS]

$(STDOBJS) $(VOBJS) $(FPOBJS) $(VAXOBJS) atoi.o atol.o:
	$(CC) -c -p $(CFLAGS) $(CINCLUDES) -Md ../$*.c
	-$(LD) -X -r -o profiled/$*.o $*.o
	$(CC) -c $(CFLAGS) $(CINCLUDES) -Mg ../$*.c
	-$(LD) -x -r -o gfloat/$*.o $*.o
	$(CC) -c $(CFLAGS) $(CINCLUDES) -Md ../$*.c
	-$(LD) -x -r $*.o
	mv a.out $*.o

clean:	cleanprofiled cleangfloat

cleanprofiled:
	-$(RM) profiled/*

cleangfloat:
	-$(RM) gfloat/*
