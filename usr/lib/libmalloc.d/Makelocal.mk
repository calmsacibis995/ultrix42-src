#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

LINTFLAGS = -p

ARFILE=	libmalloc.a

OBJS	= malloc.o

include ../Makelocal_$(MACHINE).mk

malloc.o:	malloc.c

all:	llib-lmalloc.ln

tools:
	$(INSTALL) -c -m 644 libmalloc.a ${DESTROOT}/usr/lib/libmalloc.a
	$(RANLIB) ${DESTROOT}/usr/lib/libmalloc.a

install: tools
	@-if [ ! -d ${LINTDIR} ] ; then \
		mkdir -p ${LINTDIR} ; \
		/etc/chown root ${LINTDIR} ; \
		chgrp system ${LINTDIR} ; \
		chmod 0755 ${LINTDIR} ; \
	else exit 0; \
	fi
	$(INSTALL) -c -m 644 llib-lmalloc.ln ${LINTDIR}
	$(INSTALL) -c -m 644 ../llib-lmall.c ${LINTDIR}/llib-lmalloc

include $(GMAKERULES)
