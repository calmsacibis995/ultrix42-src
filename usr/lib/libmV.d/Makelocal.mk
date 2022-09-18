#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)


CFLAGS=-O -DSYSTEM_FIVE
DESTLIB= $(DESTROOT)/usr/lib
DESTLIST= ${DESTLIB}

OBJS=asin.o atan.o erf.o fabs.o floor.o fmod.o gamma.o hypot.o\
	jn.o j0.o j1.o lgamma.o pow.o log.o sin.o sinh.o sqrt.o\
	tan.o tanh.o\
	exp.o matherr.o 

asin.o:		asin.c
atan.o:		atan.c
erf.o:		erf.c
fabs.o:		fabs.c
floor.o:	floor.c
fmod.o:		fmod.c
gamma.o:	gamma.c
hypot.o:	hypot.c
jn.o:		jn.c
j0.o:		j0.c
j1.o:		j1.c
lgamma.o:	lgamma.c
pow.o:		pow.c
log.o:		log.c
sin.o:		sin.c
sinh.o:		sinh.c
sqrt.o:		sqrt.c
tan.o:		tan.c
tanh.o:		tanh.c
exp.o:		exp.c
matherr.o:	matherr.c

include ../Makelocal_$(MACHINE).mk

libmV.a: $(OBJS)
	ar cru libmV.a $(OBJS)

libmVg.a: $(OBJS)
	cd gfloat; ar cru ../libmVg.a $(OBJS)

libmV_p.a: $(OBJS)
	cd profiled; ar cru ../libmV_p.a $(OBJS)

include $(GMAKERULES)
