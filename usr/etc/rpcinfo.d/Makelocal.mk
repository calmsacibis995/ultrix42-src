#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	rpcinfo

OBJS=	rpcinfo.o

rpcinfo.o:	rpcinfo.c

install:
	$(INSTALL) -c -s rpcinfo $(DESTROOT)/usr/etc/rpcinfo
	$(RM) $(DESTROOT)/etc/rpcinfo
	$(LN) -s ../usr/etc/rpcinfo $(DESTROOT)/etc/rpcinfo

include $(GMAKERULES)
