#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

SOBJS=	_setjmp.o abort.o abs.o alloca.o atof.o atomic_op.o \
	bcmp.o bcopy.o bzero.o \
	div.o environ.o fabs.o ffs.o insque.o ldexp.o \
	memcmp.o memcpy.o memmove.o \
	memset.o modf.o nargs.o remque.o setjmp.o udiv.o urem.o

COBJS=	ecvt.o frexp.o index.o isnan.o rindex.o \
	strcat.o strcmp.o strcpy.o strlen.o strncat.o strncpy.o

OBJS=	$(COBJS) $(SOBJS)

_setjmp.o:	_setjmp.s
abort.o:	abort.s
abs.o:		abs.s
alloca.o:	alloca.s
atof.o:		atof.s
atomic_op.o:	atomic_op.s
bcmp.o:		bcmp.s
bcopy.o:	bcopy.s
bzero.o:	bzero.s
div.o:		div.s
environ.o:	environ.s
fabs.o:		fabs.s
ffs.o:		ffs.s
insque.o:	insque.s
ldexp.o:	ldexp.s
memcmp.o:	memcmp.s
memcpy.o:	memcpy.s
memmove.o:	memmove.s
memset.o:	memset.s
modf.o:		modf.s
nargs.o:	nargs.s
remque.o:	remque.s
setjmp.o:	setjmp.s
udiv.o:		udiv.s
urem.o:		urem.s

ecvt.o:		ecvt.c
frexp.o:	frexp.c
index.o:	index.c
isnan.o:	isnan.c
rindex.o:	rindex.c
strcat.o:	strcat.c
strcmp.o:	strcmp.c
strcpy.o:	strcpy.c
strlen.o:	strlen.c
strncat.o:	strncat.c
strncpy.o:	strncpy.c

$(SOBJS):
	/lib/cpp -E -DPROF ../$*.s | $(AS) -o $*.o
	ld -X -r -o profiled/$*.o $*.o;
	/lib/cpp -E -DGFLOAT ../$*.s | $(AS) -o $*.o
	ld -x -r -o gfloat/$*.o $*.o
	/lib/cpp -E ../$*.s | $(AS) -o $*.o
	ld -x -r $*.o
	mv a.out $*.o

$(COBJS):
	cc -p -c -Md $(CFLAGS) ../$*.c
	-ld -X -r -o profiled/$*.o $*.o
	cc -c -Mg $(CFLAGS) ../$*.c
	-ld -x -r -o gfloat/$*.o $*.o
	cc -c -Md $(CFLAGS) ../$*.c
	-ld -x -r $*.o
	mv a.out $*.o

clean: cleangfloat cleanprofiled

cleangfloat:
	-$(RM) gfloat/*

cleanprofiled:
	-$(RM) profiled/*

include $(GMAKERULES)
