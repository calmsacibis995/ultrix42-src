# @(#)Makelocal_vax.mk	4.1	ULTRIX	7/3/90

CFLAGS=	-O -YPOSIX -D_POSIX_SOURCE -D_XOPEN_SOURCE

$(GENOBJS):
	$(CC) -c -p $(CFLAGS) $(CINCLUDES) -Md $*.c
	-ld -X -r -o profiled/$*.o $*.o
	$(CC) -c $(CFLAGS) $(CINCLUDES) -Mg $*.c
	-ld -x -r -o gfloat/$*.o $*.o
	$(CC) -c $(CFLAGS) $(CINCLUDES) -Md $*.c
	-ld -x -r $*.o
	mv a.out $*.o

# $(OBJS):
#	$(CC) -c -p $(CFLAGS) $(CINCLUDES) -Md ../$*.c
#	-ld -X -r -o profiled/$*.o $*.o
#	$(CC) -c $(CFLAGS) $(CINCLUDES) -Mg ../$*.c
#	-ld -x -r -o gfloat/$*.o $*.o
#	$(CC) -c $(CFLAGS) $(CINCLUDES) -Md ../$*.c
#	-ld -x -r $*.o
#	mv a.out $*.o

clean:	cleangfloat cleanprofiled

cleangfloat:
	-$(RM) gfloat/*

cleanprofiled:
	-$(RM) gfloat/*
