#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)
#
# Makefile to install pcs750.bin, vmb.exe, cibca.bin and ci780.bin
#
#
DESTROOT=
TARGET=/usr/mdec

install:

	-if [ ! -d ${DESTROOT}${TARGET} ] ; then \
		mkdir ${DESTROOT}${TARGET}; \
		chmod 755 ${DESTROOT}${TARGET} ; \
	else \
		true; \
	fi
	install -c ../pcs750.bin ${DESTROOT}${TARGET}/pcs750.bin
	install -c ../pcs750.bin ${DESTROOT}/pcs750.bin
	install -c ../vmb.exe ${DESTROOT}${TARGET}/vmb.exe
	install -c ../vmb.exe ${DESTROOT}/vmb.exe
	install -c ../ci780.bin ${DESTROOT}${TARGET}/ci780.bin
	install -c ../cibca.bin ${DESTROOT}${TARGET}/cibca.bin

tools1:

	-if [ ! -d ${DESTROOT}${TARGET} ] ; then \
		mkdir ${DESTROOT}${TARGET}; \
		chmod 755 ${DESTROOT}${TARGET} ; \
	else \
		true; \
	fi
	install -c vmb.exe ${DESTROOT}${TARGET}/vmb.exe
	install -c ci780.bin ${DESTROOT}${TARGET}/ci780.bin
	install -c cibca.bin ${DESTROOT}${TARGET}/cibca.bin
	install -c pcs750.bin ${DESTROOT}${TARGET}/pcs750.bin


include $(GMAKERULES)
