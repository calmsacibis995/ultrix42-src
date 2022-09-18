#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	reset.o

reset.o: reset.c
	/lib/cpp -E -DPROF ../$*.c | ${AS} -o $*.o
	-ld -X -r -o profiled/$*.o $*.o
	/lib/cpp -E ../$*.c | ${AS} -o $*.o
	-ld -x -r $*.o
	mv a.out $*.o
	$(RM) gfloat/$*.o
	ln $*.o gfloat/$*.o

clean: cleangfloat cleanprofiled

cleangfloat:
	-$(RM) gfloat/*

cleanprofiled:
	-$(RM) profiled/*

include $(GMAKERULES)
