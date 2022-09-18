#@(#)Makefile	4.1	Ultrix	7/17/90
#  Makefile for total Modula2 system.
#
# Read the file ReadMe for directions on how to build the system.
#
DESTROOT=
.DEFAULT: all

#
# the 'make all' is done when just a 'make' is done

tools2:
	-if [ ! -d ${DESTROOT}/usr/lib/mod ]; \
	then \
		mkdir ${DESTROOT}/usr/lib/mod; \
		chmod 755 ${DESTROOT}/usr/lib/mod; \
	else \
		true; \
	fi
	-if [ ! -d lib/prof ]; \
	then \
		mkdir lib/prof; \
		chmod 755 lib/prof; \
	else \
		true; \
	fi
	(cd mod       ; make ${MFLAGS} DESTROOT=${DESTROOT} install)
	(cd mod2      ; make ${MFLAGS} DESTROOT=${DESTROOT} install)
	(cd xlate     ; make ${MFLAGS} DESTROOT=${DESTROOT} install)
	(cd lib       ; make ${MFLAGS} DESTROOT=${DESTROOT} install)

all:
	-if [ ! -d lib/prof ]; \
	then \
		mkdir lib/prof; \
		chmod 755 lib/prof; \
	else \
		true; \
	fi
	(cd mod       ; make ${MFLAGS} imod)
	(cd mod2      ; make ${MFLAGS} all)
	(cd xlate     ; make ${MFLAGS} xlate)
	(cd lib       ; make ${MFLAGS} all)
	(cd imc       ; make ${MFLAGS} iimc)
	(cd p2m2      ; make ${MFLAGS} p2m2)
#	(cd doc       ; make ${MFLAGS} all)

install:
	-if [ ! -d ${DESTROOT}/usr/lib/mod ]; \
	then \
		mkdir ${DESTROOT}/usr/lib/mod; \
		chmod 755 ${DESTROOT}/usr/lib/mod; \
	else \
		true; \
	fi
	(cd mod       ; make ${MFLAGS} DESTROOT=${DESTROOT} install)
	(cd mod2      ; make ${MFLAGS} DESTROOT=${DESTROOT} install)
	(cd xlate     ; make ${MFLAGS} DESTROOT=${DESTROOT} install)
	(cd lib       ; make ${MFLAGS} DESTROOT=${DESTROOT} install)
	(cd imc       ; make ${MFLAGS} DESTROOT=${DESTROOT} install)
	(cd p2m2      ; make ${MFLAGS} DESTROOT=${DESTROOT} install)

	
clean:
	(cd mod      ; make ${MFLAGS} clean)
	(cd mod2     ; make ${MFLAGS} clean)
	(cd xlate    ; make ${MFLAGS} clean)
	(cd lib      ; make ${MFLAGS} clean)
	(cd imc      ; make ${MFLAGS} clean)
	(cd p2m2     ; make ${MFLAGS} clean)
	(cd doc      ; make ${MFLAGS} clean)

clobber:	clean
	(cd mod      ; rm -f  *[a-z])
	(cd mod2     ; rm -f  *[a-z2])
	(cd xlate    ; rm -f  *[a-z])
	(cd lib      ; rm -f  -r *[a-z])
	(cd imc      ; rm -f  *[a-z])
	(cd p2m2     ; rm -f  *[a-z2])
	(cd test     ; rm -f  *[a-z])
	(cd doc      ; rm -f  *[a-z])

sccsinfo:
	sccs info
	(cd mod      ; sccs info)
	(cd mod2     ; sccs info)
	(cd xlate    ; sccs info)
	(cd lib      ; sccs info)
	(cd imc      ; sccs info)
	(cd p2m2     ; sccs info)
	(cd test     ; sccs info)
	(cd doc      ; sccs info)

sccsget:
	(cd mod      ; sccs get SCCS)
	(cd mod2     ; sccs get SCCS)
	(cd xlate    ; sccs get SCCS)
	(cd lib      ; sccs get SCCS)
	(cd imc      ; sccs get SCCS)
	(cd p2m2     ; sccs get SCCS)
	(cd test     ; sccs get SCCS)
	(cd doc      ; sccs get SCCS)
