#  @(#)Makelocal.mk	4.1  ULTRIX  8/8/90

include $(GMAKEVARS)

ARFILE=	libaud.a

OBJS=	trustedevents.o audgenl.o

all:	libaud.a
libaud.a:	${OBJS}
trustedevents.o:	trustedevents.c
audgenl.o:	audgenl.c

install:
	install -c -m 644 libaud.a ${DESTROOT}/usr/lib
	ranlib ${DESTROOT}/usr/lib/libaud.a

include $(GMAKERULES)
