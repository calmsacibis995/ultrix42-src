#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
include $(GMAKEVARS)
OBJECTS=getnodeent.o putnodeent.o vio.o dnet_addr.o dnet_getloca.o

all: libmop.a
libmop.a: ${OBJECTS}
	ar cr libmop.a ${OBJECTS}

tools2 : libmop.a
tools2 install:
	install -c -m 644 libmop.a $(DESTROOT)/usr/lib
	ranlib $(DESTROOT)/usr/lib/libmop.a

include $(GMAKERULES)

getnodeent.o:	getnodeent.c
putnodeent.o:	putnodeent.c
vio.o:	vio.c
dnet_addr.o:	dnet_addr.c
dnet_getloca.o:	dnet_getloca.c
