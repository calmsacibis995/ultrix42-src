# *sccsid = "@(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90"
include $(GMAKEVARS)

all: liberrlog.a

liberrlog.a: window.o logerr.o
	$(RM) liberrlog.a
	ar rvc liberrlog.a window.o logerr.o
	$(RM) window.o logerr.o

window.o:	window.c
logerr.o:	logerr.c

tools2: all
tools2 install:
	install -c liberrlog.a ${DESTROOT}/usr/lib
	ranlib ${DESTROOT}/usr/lib/liberrlog.a

include $(GMAKERULES)
