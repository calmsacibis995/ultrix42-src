#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

#
# /usr/examples/dli Makefile
#

DESTLIST= ${DESTROOT}/usr/examples/dli

STD = dli_802.c dli_802d.c dli_eth.c dli_ethd.c dli_setsockopt.c

install:
	for i in ${STD}; do \
	${INSTALL} -c -m 444 ../$$i ${DESTROOT}/usr/examples/dli/$$i; \
	done

include $(GMAKERULES)
