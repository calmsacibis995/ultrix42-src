#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
include $(GMAKEVARS)
CFLAGS=	-O -DCM_N -DCM_GT -DCM_B -DCM_D
LD = ld
OBJECTS=	termcap.o tgoto.o tputs.o


all:	$(MACHINE)all

vaxall: termcap.a termcap_p.a

mipsall: termcap.a

termcap.a: termcap.o tgoto.o tputs.o
	ar cr termcap.a termcap.o tgoto.o tputs.o

termcap_p.a: termcap.o tgoto.o tputs.o
	cd profiled; ar cr ../termcap_p.a termcap.o tgoto.o tputs.o

termcap.o:	termcap.c
tgoto.o:	tgoto.c
tputs.o:	tputs.c

tools2 install:	$(MACHINE)install

mipsinstall:
	install -c termcap.a ${DESTROOT}/usr/lib/libtermcap.a
	-rm -f ${DESTROOT}/usr/lib/libtermlib.a
	ln ${DESTROOT}/usr/lib/libtermcap.a ${DESTROOT}/usr/lib/libtermlib.a
	ranlib ${DESTROOT}/usr/lib/libtermcap.a

vaxinstall:
	install -c termcap.a ${DESTROOT}/usr/lib/libtermcap.a
	-rm -f ${DESTROOT}/usr/lib/libtermlib.a
	ln ${DESTROOT}/usr/lib/libtermcap.a ${DESTROOT}/usr/lib/libtermlib.a
	ranlib ${DESTROOT}/usr/lib/libtermcap.a
	install -c termcap_p.a ${DESTROOT}/usr/lib/libtermcap_p.a
	-rm -f ${DESTROOT}/usr/lib/libtermlib_p.a
	ln ${DESTROOT}/usr/lib/libtermcap_p.a ${DESTROOT}/usr/lib/libtermlib_p.a
	ranlib ${DESTROOT}/usr/lib/libtermcap_p.a

pretools tools1:
	for i in ${OBJECTS}; do \
		echo $$i; \
		$(CCCMD) ../`basename $$i .o`.c; \
		$(CC) $(LDFLAGS) -X -r $$i; \
		mv a.out $$i; \
	done
	rm -f termcap.a
	ar cr termcap.a ${OBJECTS}
	rm -f ${OBJECTS}
	install -c termcap.a ${DESTROOT}/usr/lib/libtermcap.a
	-if [ -f ${DESTROOT}/usr/lib/libtermlib.a ]; \
	then \
		rm -f ${DESTROOT}/usr/lib/libtermlib.a; \
	else \
		true; \
	fi
	ln ${DESTROOT}/usr/lib/libtermcap.a ${DESTROOT}/usr/lib/libtermlib.a
	ranlib ${DESTROOT}/usr/lib/libtermcap.a

clean$(MACHINE): cleanprofiled
cleanprofiled:
	-$(RM) -r profiled

include $(GMAKERULES)
include ../Makelocal_$(MACHINE).mk

