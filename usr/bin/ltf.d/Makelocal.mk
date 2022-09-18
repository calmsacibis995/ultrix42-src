#  @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)
HDRS=  filetypes.h ltfdefs.h ltferrs.h 
CINCLUDES=-I..
AOUT=ltf
OBJS=	filenames.o filetype.o initvol.o ltf.o ltfvars.o mstrcmp.o \
	odm.o putdir.o putfile.o scantape.o statchk.o xtractf.o

filenames.o:	filenames.c
filetype.o:	filetype.c
initvol.o:	initvol.c
ltf.o:	ltf.c
ltfvars.o:	ltfvars.c
mstrcmp.o:	mstrcmp.c
odm.o:	odm.c
putdir.o:	putdir.c
putfile.o:	putfile.c
scantape.o:	scantape.c
statchk.o:	statchk.c
xtractf.o:	xtractf.c

install:
	install -c -s ltf ${DESTROOT}/usr/bin/ltf
	$(RM) ${DESTROOT}/bin/ltf
	ln -s ../usr/bin/ltf ${DESTROOT}/bin/ltf

include $(GMAKERULES)
