#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

SUBDIRS= _$(MACHINE).d
DESTROOT=
TARGET=/usr/mdec/named_pipe

install:

	-if [ ! -d ${DESTROOT}${TARGET} ] ; then \
		mkdir ${DESTROOT}${TARGET}; \
		chmod 755 ${DESTROOT}${TARGET} ; \
	else \
		true; \
	fi
	install -c ../nfs_subr.o.31mips ${DESTROOT}${TARGET}/nfs_subr.o.31mips 
	install -c ../nfs_subr.o.31vax ${DESTROOT}${TARGET}/nfs_subr.o.31vax 
	install -c ../vnodeops_gfs.o.31mips ${DESTROOT}${TARGET}/vnodeops_gfs.o.31mips 
	install -c ../vnodeops_gfs.o.31vax ${DESTROOT}${TARGET}/vnodeops_gfs.o.31vax 


include $(GMAKERULES)
