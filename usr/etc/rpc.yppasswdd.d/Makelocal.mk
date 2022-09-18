#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	rpc.yppasswdd

OBJS=	rpc.yppasswdd.o

rpc.yppasswdd.o:	rpc.yppasswdd.c

install:
	$(INSTALL) -c -s rpc.yppasswdd $(DESTROOT)/usr/etc/rpc.yppasswdd
	$(RM) $(DESTROOT)/etc/rpc.yppasswdd
	$(LN) -s ../usr/etc/rpc.yppasswdd $(DESTROOT)/etc/rpc.yppasswdd

include $(GMAKERULES)
