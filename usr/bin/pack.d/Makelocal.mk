#	@(#)Makelocal.mk	4.1	Ultrix	7/17/90
#	Makefile for pack, unpack, pcat
#	Derived from generic Make.build.only template
#	Only tricky thing here is that pcat is a link to unpack
include $(GMAKEVARS)

OBJECTS = pack.o unpack.o
STD = pack unpack

all: ${STD}

pack:	../pack.c
	${LDCMD} ${CFLAGS} ../pack.c

unpack: ../unpack.c
	${LDCMD} ${CFLAGS} ../unpack.c

install:
	for i in ${STD}; do \
	(install ${INSTFLAGS} -c -s $$i ${DESTROOT}/usr/bin/$$i); done
	$(RM) ${DESTROOT}/usr/bin/pcat
	ln ${DESTROOT}/usr/bin/unpack ${DESTROOT}/usr/bin/pcat


include $(GMAKERULES)
