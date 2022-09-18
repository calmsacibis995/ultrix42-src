#  @(#)Makelocal_mips.mk	4.1  ULTRIX  7/3/90

LD=ld

$(OBJS):
	$(CCCMD) -G 0 ../$<
	$(MV) $*.o G0/$*.o
	$(CCCMD) -DAUTHEN ../$<
	-$(LD) -x -r -o ../_$(MACHINE)krb.b/$*.o $*.o
	$(CCCMD) ../$<
	-$(LD) -x -r $*.o
	mv a.out $*.o

clean: cleanG0 cleankrb

cleanG0:
	-$(RM) G0/*

cleankrb:
	-$(RM) ../_mipskrb.b/*
