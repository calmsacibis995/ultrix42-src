#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=${DESTROOT}/usr/new
DESTROOT=
CFLAGS=	-O
LIB=	../../../bin/tip.d/aculib/_$(MACHINE).b/aculib.a
SRCS=	slattach.c
OBJS=	slattach.o
LOADLIBES = -ldbm

all: slattach

slattach.o: slattach.c

slattach:	${OBJS} ${LIB}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LIB}

install: slattach
	@-if [ ! -d ${DESTROOT}/usr/new ] ; then \
		mkdir ${DESTROOT}/usr/new; \
		chmod 755 ${DESTROOT}/usr/new; \
		/etc/chown root ${DESTROOT}/usr/new; \
		chgrp system ${DESTROOT}/usr/new; \
		else true; \
		fi
	$(INSTALL) -c -s -m 4755 slattach ${DESTROOT}/usr/new/slattach

include $(GMAKERULES)
