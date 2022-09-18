# @(#)Makelocal.mk	4.1   (ULTRIX)        7/17/90
include $(GMAKEVARS)

include ../Makelocal_$(MACHINE).mk
OBJS= file.o filetype.o fregex.o
AOUT=file

filetype.o: filetype.c filetype.h
fregex.o: fregex.c
file.o: file.c 

install:
	install -c -s file ${DESTROOT}/usr/bin
	-if [ ! -d $(DESTROOT)/usr/lib/file ] ; then \
		mkdir ${DESTROOT}/usr/lib/file; \
		chmod 755 ${DESTROOT}/usr/lib/file ; \
		/etc/chown root ${DESTROOT}/usr/lib/file ; \
		chgrp system ${DESTROOT}/usr/lib/file ; \
	else true ; \
	fi
	install -c -o root -m 444 ../magic ${DESTROOT}/usr/lib/file/magic

include $(GMAKERULES)
