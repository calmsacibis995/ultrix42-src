#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
#
# Makefile to copy files in pub directory 
#

include $(GMAKEVARS)

FILES=  greek eqnchar ascii

install:
	-if [ ! -d ${DESTROOT}/usr/pub ]; \
	then \
		mkdir ${DESTROOT}/usr/pub; \
		chmod 755 ${DESTROOT}/usr/pub; \
	else \
		true; \
	fi
	for i in ${FILES} ; do \
		install -c ../$$i ${DESTROOT}/usr/pub/$$i ; \
	done


include $(GMAKERULES)
