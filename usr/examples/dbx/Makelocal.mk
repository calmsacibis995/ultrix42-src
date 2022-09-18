# @(#)Makelocal.mk	4.1	(ULTRIX)	1/25/91

include $(GMAKEVARS)

#
# /usr/examples/dbx Makefile
#

DESTLIST= ${DESTROOT}/usr/examples/dbx

STD = dbx_sample.c

install:
	for i in ${STD}; do \
	${INSTALL} -c -m 444 ../$$i ${DESTROOT}/usr/examples/dbx/$$i; \
	done

include $(GMAKERULES)
