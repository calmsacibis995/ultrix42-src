#  @(#)Makelocal.mk	4.1  ULTRIX  2/21/91

include $(GMAKEVARS)

#
# /usr/examples/devdrivers Makefile
#

DESTLIST= ${DESTROOT}/usr/examples/devdrivers

STD = vmedma.c vmemmap.c tcmmap.c

install:
	for i in ${STD}; do \
	${INSTALL} -c -m 444 ../$$i ${DESTROOT}/usr/examples/devdrivers/$$i; \
	done

include $(GMAKERULES)
