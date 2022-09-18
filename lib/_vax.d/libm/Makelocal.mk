#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

OBJS=	acosh.o asincos.o asinh.o atan.o atanh.o cosh.o erf.o \
	exp.o exp__E.o expm1.o floor.o fmod.o gamma.o j0.o j1.o jn.o \
	lgamma.o log.o log10.o log1p.o log__L.o pow.o sinh.o tanh.o 

DOBJS=	argred.o d_atan2.o d_cabs.o d_cbrt.o infnan.o sincos.o sqrt.o \
	d_support.o tan.o

GOBJS=	atan2.o cabs.o cbrt.o support.o trig.o

all: libm.a libm_p.a libmg.a

libm.a libm_p.a : $(OBJS) $(DOBJS) fabs.o
	cd profiled; ar cru ../libm_p.a $(OBJS) $(DOBJS) fabs.o
	ar cru libm.a $(OBJS) $(DOBJS) fabs.o

libmg.a : $(OBJS) $(GOBJS) fabs.o
	cd gfloat; ar cru ../libmg.a $(OBJS) $(GOBJS) fabs.o

fabs.o : fabs.s mcount.sed
	sed -f ../mcount.sed ../fabs.s | as -o fabs.o
	ld -x -r fabs.o
	mv a.out profiled/fabs.o
	as -o fabs.o ../fabs.s
	ld -x -r fabs.o
	mv a.out fabs.o
	cp fabs.o gfloat/fabs.o

clean:	cleangfloat cleanprofiled

cleangfloat:
	$(RM) gfloat/*

cleanprofiled:
	$(RM) profiled/*

install: libm.a libm_p.a libmg.a
	${INSTALL} -c libmg.a ${DESTROOT}/usr/lib
	ranlib ${DESTROOT}/usr/lib/libmg.a
	${INSTALL} -c libm.a ${DESTROOT}/usr/lib
	ranlib ${DESTROOT}/usr/lib/libm.a
	${INSTALL} -c libm_p.a ${DESTROOT}/usr/lib
	ranlib ${DESTROOT}/usr/lib/libm_p.a

acosh.o:	acosh.c
asincos.o:	asincos.c
asinh.o:	asinh.c
atan.o:		atan.c
atanh.o:	atanh.c
cosh.o:		cosh.c
erf.o:		erf.c
exp.o:		exp.c
exp__E.o:	exp__E.c
expm1.o:	expm1.c
floor.o:	floor.c
fmod.o:		fmod.c
gamma.o:	gamma.c
j0.o:		j0.c
j1.o:		j1.c
jn.o:		jn.c
lgamma.o:	lgamma.c
log.o:		log.c
log10.o:	log10.c
log1p.o:	log1p.c
log__L.o:	log__L.c
pow.o:		pow.c
sinh.o:		sinh.c
tanh.o:		tanh.c
argred.o:	argred.s
d_atan2.o:	d_atan2.s
d_cabs.o:	d_cabs.s
d_cbrt.o:	d_cbrt.s
infnan.o:	infnan.s
sincos.o:	sincos.s
sqrt.o:		sqrt.s
d_support.o:	d_support.s
tan.o:		tan.s
atan2.o:	atan2.c
cabs.o:		cabs.c
cbrt.o:		cbrt.c
support.o:	support.c
trig.o:		trig.c

include $(GMAKERULES)

.c.o:
	$(CC) -p -DVAX ${CFLAGS} -c ../$*.c
	-ld -X -r -o profiled/$*.o $*.o
	$(CC) -c -Mg -DIEEE ${CFLAGS} ../$*.c
	-ld -x -r -o gfloat/$*.o $*.o
	$(CC) -c -DVAX ${CFLAGS} ../$*.c
	-ld -x -r $*.o
	mv a.out $*.o

.s.o:
	sed -f ../mcount.sed ../$*.s | as -o $*.o
	ld -x -r $*.o
	mv a.out profiled/$*.o
	as -o $*.o ../$*.s
	ld -x -r $*.o
	mv a.out $*.o
