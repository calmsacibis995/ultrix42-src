#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc ${DESTROOT}/etc

AOUT=	dump

OBJS=	dumpitime.o dumpmain.o dumpoptr.o dumptape.o dumptraverse.o \
	unctime.o l3tol.o

dumpitime.o:		dumpitime.c
dumpmain.o:		dumpmain.c
dumpoptr.o:		dumpoptr.c
dumptape.o:		dumptape.c
dumptraverse.o:		dumptraverse.c
unctime.o:		unctime.c
l3tol.o:		l3tol.c

install:
	$(INSTALL) -c -s dump $(DESTROOT)/usr/etc/dump.4.1
	$(RM) $(DESTROOT)/etc/dump.4.1
	$(LN) -s ../usr/etc/dump.4.1 ${DESTROOT}/etc/dump.4.1

include $(GMAKERULES)
