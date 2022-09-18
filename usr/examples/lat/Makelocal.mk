#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

#
# /usr/examples/lat Makefile
#

DESTLIST= ${DESTROOT}/usr/examples/lat

STD = dial.c latdate.c latdlogin.c

install:
	for i in ${STD}; do \
	${INSTALL} -c -m 444 ../$$i ${DESTROOT}/usr/examples/lat/$$i; \
	done

include $(GMAKERULES)
