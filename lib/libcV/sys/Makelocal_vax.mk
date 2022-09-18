#  @(#)Makelocal_vax.mk	4.1	ULTRIX	7/3/90
CFLAGS=	-O -Y

$(OBJSMISC):
	$(CC) -c -p $(CFLAGS) -Md $*.c
	-ld -X -r -o profiled/$*.o $*.o
	$(CC) -c $(CFLAGS) -Mg $*.c
	-ld -x -r -o gfloat/$*.o $*.o
	$(CC) -c $(CFLAGS) -Md $*.c
	-ld -x -r $*.o
	mv a.out $*.o

$(OBJS):
	$(CC) -c -p $(CFLAGS) -Md ../$*.c
	-ld -X -r -o profiled/$*.o $*.o
	$(CC) -c $(CFLAGS) -Mg ../$*.c
	-ld -x -r -o gfloat/$*.o $*.o
	$(CC) -c $(CFLAGS) -Md ../$*.c
	-ld -x -r $*.o
	mv a.out $*.o

clean:	cleangfloat cleanprofiled

cleangfloat:
	-$(RM) gfloat/*

cleanprofiled:
	-$(RM) gfloat/*
