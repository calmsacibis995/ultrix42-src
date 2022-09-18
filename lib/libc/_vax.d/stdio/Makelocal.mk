#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	doprnt.o

doprnt.o: doprnt.s
	cp ../doprnt.s doprnt.c
	cc -E doprnt.c | sed -f ../mcount.sed | as -o doprnt.o
	-ld -x -r -o profiled/doprnt.o doprnt.o
	cc -E -DGFLOAT doprnt.c | as -o doprnt.o
	-ld -x -r -o gfloat/doprnt.o doprnt.o
	cc -E doprnt.c | as -o doprnt.o
	-ld -x -r doprnt.o
	mv a.out doprnt.o
	rm -f doprnt.c

clean: cleangfloat cleanprofiled

cleangfloat:
	-$(RM) gfloat/*

cleanprofiled:
	-$(RM) profiled/*

include $(GMAKERULES)
