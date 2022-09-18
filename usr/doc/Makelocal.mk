#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
#
# Makefile to move doc.C around
#
# doc.C is a compressed tar image containing the /usr/doc
# files.  These are not supported or maintained by DIGITAL
#
#
include $(GMAKEVARS)

FILES = README Makefile

install:
	-if [ ! -d ${DESTROOT}/usr/doc ]; \
	then \
	    mkdir ${DESTROOT}/usr/doc; \
	else \
	    true; \
	fi
	install -c ../README ${DESTROOT}/usr/doc/README
	install -c ../doc.C ${DESTROOT}/usr/doc/doc.C



include $(GMAKERULES)
