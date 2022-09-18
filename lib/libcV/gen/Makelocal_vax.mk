#  @(#)Makelocal_vax.mk	4.2	ULTRIX	8/13/90
CFLAGS=	-O -Y

$(GENOBJS) getpw.o:
	$(CC) -c -p $(CFLAGS) $(CINCLUDES) -Md $*.c
	-ld -X -r -o profiled/$*.o $*.o
	$(CC) -c $(CFLAGS) $(CINCLUDES) -Mg $*.c
	-ld -x -r -o gfloat/$*.o $*.o
	$(CC) -c $(CFLAGS) $(CINCLUDES) -Md $*.c
	-ld -x -r $*.o
	mv a.out $*.o

$(OBJS):
	$(CC) -c -p $(CFLAGS) $(CINCLUDES) -Md ../$*.c
	-ld -X -r -o profiled/$*.o $*.o
	$(CC) -c $(CFLAGS) $(CINCLUDES) -Mg ../$*.c
	-ld -x -r -o gfloat/$*.o $*.o
	$(CC) -c $(CFLAGS) $(CINCLUDES) -Md ../$*.c
	-ld -x -r $*.o
	mv a.out $*.o

clean:	cleangfloat cleanprofiled

cleangfloat:
	-$(RM) gfloat/*

cleanprofiled:
	-$(RM) profiled/*
