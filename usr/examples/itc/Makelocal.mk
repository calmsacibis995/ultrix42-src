#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

#
# /usr/examples/itc Makefile
#

DESTLIST= ${DESTROOT}/usr/examples/itc

STD = itc.c

install:
	for i in ${STD}; do \
	${INSTALL} -c -m 444 ../$$i ${DESTROOT}/usr/examples/itc/$$i; \
	done

include $(GMAKERULES)
