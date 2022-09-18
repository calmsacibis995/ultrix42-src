#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

FILES = README fonts.C

DESTLIST=${DESTROOT}/usr/lib/vfont

install:
	@for i in ${FILES}; do \
		$(INSTALL) -c ../$$i ${DESTLIST}/$$i; \
	done

include $(GMAKERULES)
