#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
include $(GMAKEVARS)

INSTALL= install

FILES=	acos.o asin.o atan.o cbrt.o pow.o exp.o log.o sin.o \
	sinh.o sqrt.o tan.o tanh.o

all:	libnm.a libnm_p.a
libnm.a libnm_p.a : $(FILES)
	cd profiled; ar cru ../libnm_p.a $(FILES)
	ar cru libnm.a $(FILES)

tools2: libnm.a libnm_p.a
tools2 install:
	${INSTALL} -c libnm.a ${DESTROOT}/usr/lib/libnm.a
	ranlib ${DESTROOT}/usr/lib/libnm.a
	${INSTALL} -c libnm_p.a ${DESTROOT}/usr/lib/libnm_p.a
	ranlib ${DESTROOT}/usr/lib/libnm_p.a

acos.o:	acos.c
asin.o:	asin.s
atan.o:	atan.s
cbrt.o:	cbrt.s
pow.o:	pow.c
exp.o:	exp.s
log.o:	log.s
sin.o:	sin.s
sinh.o:	sinh.s
sqrt.o:	sqrt.s
tan.o:	tan.s
tanh.o:	tanh.s

clean$(MACHINE): cleanprofiled
cleanprofiled:
	-$(RM) -r profiled
include $(GMAKERULES)

.c.o:
	-@if [ ! -d profiled ] ; then \
		mkdir profiled; \
	fi
	${CCCMD} -p ../$*.c
	-ld -X -r $*.o
	mv a.out profiled/$*.o
	${CCCMD} ../$*.c
	-ld -x -r $*.o
	mv a.out $*.o

.s.o:
	-@if [ ! -d profiled ] ; then \
		mkdir profiled; \
	fi
	sed -f ../mcount.sed ../$*.s | ${AS} -o $*.o
	-ld -X -r $*.o
	mv a.out profiled/$*.o
	${AS} -o $*.o ../$*.s
	-ld -x -r $*.o
	mv a.out $*.o

