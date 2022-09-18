#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	tzX.o

tzX.o:		tzX.s

$(OBJS):
	/lib/cpp -E -DPROF  ../$*.s | ${AS} -o $*.o
	-ld -x -r -o profiled/$*.o $*.o
	/lib/cpp -E  ../$*.s | ${AS} -o $*.o
	-ld -x -r -o gfloat/$*.o $*.o
	/lib/cpp -E  ../$*.s | ${AS} -o $*.o
	-ld -x -r $*.o
	mv a.out $*.o

clean:	cleangfloat cleanprofiled

cleangfloat:
	-$(RM) gfloat/*

cleanprofiled:
	-$(RM) profiled/*

include $(GMAKERULES)
