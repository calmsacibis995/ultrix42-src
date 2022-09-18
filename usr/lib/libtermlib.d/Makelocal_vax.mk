#  @(#)Makelocal_vax.mk	4.1  ULTRIX  7/2/90
.c.o:
	-@if [ ! -d profiled ] ; then \
		mkdir profiled; \
	fi
	$(CCCMD) -p ../$*.c
	$(LD) $(LDFLAGS) -x -r -o profiled/$*.o $*.o
	$(CCCMD) ../$*.c
	$(LD) $(LDFLAGS) -X -r $*.o
	mv a.out $*.o
