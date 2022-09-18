# @(#)Makelocal.mk	4.5 (ULTRIX) 1/25/91

include $(GMAKEVARS)

CDEFINES = -DULTRIX -DNLstrncpy=strncpy
LIB	= libi.a
CFLAGS = -YPOSIX

MSGBASE = ${DESTROOT}/usr/lib/nls

OBJECTS	= nl_ctime.o nl_init.o nl_langinfo.o nl_string.o printf.o \
	  scanf.o _idoprnt.o _idoscan.o \
	  catopen.o catgets.o catgetmsg.o dordmsg.o fcatgets.o fcatgetmsg.o

all: ${LIB}

${LIB}: $(OBJECTS)
	ar cr ${LIB} ${OBJECTS}
	ranlib ${LIB}

pretools tools1 tools2:	all
pretools tools1 tools2 install:
	install -c -m 444 -o bin -g system ${LIB} ${DESTROOT}/usr/lib
	-ranlib ${DESTROOT}/usr/lib/libi.a

	mkdir -p ${MSGBASE}/msg
	chmod -R 755 ${MSGBASE}
	

conv.o:			conv.c
nl_ctime.o:		nl_ctime.c
nl_init.o:		nl_init.c
nl_langinfo.o:		nl_langinfo.c
nl_string.o:		nl_string.c
printf.o:		printf.c
scanf.o:		scanf.c
_idoprnt.o:		_idoprnt.c
_idoscan.o:		_idoscan.c
catopen.o:		catopen.c
catgets.o:		catgets.c
catgetmsg.o:		catgetmsg.c
dordmsg.o:		dordmsg.c
fcatgets.o:		fcatgets.c
fcatgetmsg.o:		fcatgetmsg.c

include $(GMAKERULES)

