#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	htonl.o htons.o ntohl.o ntohs.o

htonl.o:	htonl.c
htons.o:	htons.c
ntohl.o:	ntohl.c
ntohs.o:	ntohs.c

$(OBJS):
	/lib/cpp -E -DPROF ../$*.c | $(AS) -o $*.o
	-ld -x -r $*.o
	mv a.out profiled/$*.o
	/lib/cpp -E ../$*.c | $(AS) -o $*.o
	-ld -x -r $*.o
	mv a.out $*.o
	-rm gfloat/$*.o
	-ln $*.o gfloat/$*.o

clean: cleangfloat cleanprofiled

cleangfloat:
	$(RM) gfloat/*

cleanprofiled:
	$(RM) profiled/*

include $(GMAKERULES)
