# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90
#
#	@(#)Makefile	4.3	(Berkeley)	83/06/19
#
include $(GMAKEVARS)
CFLAGS = -O
CINCLUDES= -I. -I..

DESTLIST= ${DESTROOT}/usr/lib/learn ${DESTROOT}/usr/lib/learn/bin 

OBJS = copy.o dounit.o getlesson.o learn.o list.o mem.o \
	makpipe.o maktee.o mysys.o selsub.o selunit.o \
	start.o whatnow.o wrapup.o
AOUT=learn

LLIB	= ${DESTROOT}/usr/lib/learn/bin

all:	learn lrntee lcount

$(OBJS): lrnref.h

lcount lrntee:
	$(LDCMD) $(CFLAGS) ../$@.c

lcount:	../lcount.c
lrntee:	../lrntee.c

copy.o:		copy.c
dounit.o:	dounit.c
getlesson.o:	getlesson.c
learn.o:	learn.c
list.o:		list.c
mem.o:		mem.c
makpipe.o: 	makpipe.c
maktee.o:	maktee.c
mysys.o:	mysys.c
selsub.o:	selsub.c
selunit.o:	selunit.c
start.o:	start.c
whatnow.o:	whatnow.c
wrapup.o:	wrapup.c

install:
	install -c -s learn ${DESTROOT}/usr/bin
	install -c -s learn ${LLIB}
	install -c -s lrntee ${LLIB}
	install -c -s lcount ${LLIB}

include $(GMAKERULES)
