#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

ARFILE=	libauth.a

OBJS=	getauthent.o auth.o authconfig.o storepwent.o authenticate.o checkpass.o

all:	libauth.a
libauth.a:	auth.h ${OBJS}
auth.o:		auth.c
getauthent.o:	getauthent.c
authconfig.o:	authconfig.c
storepwent.o:	storepwent.c
authenticate.o:	authenticate.c
checkpass.o:	checkpass.c

tools1 tools2:	libauth.a
tools1 tools2 install:
	install -c -m 644 libauth.a ${DESTROOT}/usr/lib
	ranlib ${DESTROOT}/usr/lib/libauth.a
	cp ../auth.h .
	install -c -m 644 auth.h ${DESTROOT}/usr/include

include $(GMAKERULES)
