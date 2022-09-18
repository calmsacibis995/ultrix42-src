#  @(#)Makelocal.mk	4.1  ULTRIX  11/13/90

include $(GMAKEVARS)

#
# /usr/examples/xti Makefile
#

DESTLIST= ${DESTROOT}/usr/examples/xti

STD = cots_server.c cots_client.c clts_server.c clts_client.c

install:
	for i in ${STD}; do \
	${INSTALL} -c -m 444 ../$$i ${DESTROOT}/usr/examples/xti/$$i; \
	done

include $(GMAKERULES)
