# @(#)Makelocal_vax.mk	4.1   (ULTRIX)        7/3/90

LD=ld

$(OBJS):
	$(CC) -c -p $(CFLAGS) -Md -DAUTHEN ../$*.c
	-$(LD) -X -r -o ../_$(MACHINE)krb.b/profiled/$*.o $*.o
	$(CC) -c $(CFLAGS) -Mg -DAUTHEN ../$*.c
	-$(LD) -x -r -o ../_$(MACHINE)krb.b/gfloat/$*.o $*.o
	$(CC) -c $(CFLAGS) -Md -DAUTHEN ../$*.c
	-$(LD) -x -r -o ../_$(MACHINE)krb.b/$*.o $*.o
	$(CC) -c -p $(CFLAGS) -Md ../$*.c
	-$(LD) -X -r -o profiled/$*.o $*.o
	$(CC) -c $(CFLAGS) -Mg ../$*.c
	-$(LD) -x -r -o gfloat/$*.o $*.o
	$(CC) -c $(CFLAGS) -Md ../$*.c
	-$(LD) -x -r $*.o
	mv a.out $*.o

clean:	cleanprofiled cleangfloat

cleanprofiled:
	-$(RM) profiled/* ../_$(MACHINE)krb.b/profiled/*

cleangfloat:
	-$(RM) gfloat/* ../_$(MACHINE)krb.b/gfloat/*
