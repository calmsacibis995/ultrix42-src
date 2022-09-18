#  @(#)Makelocal.mk	4.2  ULTRIX  8/8/90

include $(GMAKEVARS)

OBJS=	passwd.o random.o randomword.o makeseed.o

AUTHLIB=	-lauth

KRBLIBS=	-lckrb -lkrb -lknet -ldes 

all:	passwd 

passwd:		$(OBJS)
		$(LDCMD) passwd.o random.o randomword.o makeseed.o $(KRBLIBS) $(AUTHLIB)

passwd.o:	passwd.c
	$(CCCMD) -DAUTHEN ../passwd.c

random.o:	random.c

randomword.o:	randomword.c

makeseed.o:	makeseed.c

install:
	$(INSTALL) -c -s -o root -m 4755 passwd $(DESTROOT)/usr/bin/passwd
	$(RM) $(DESTROOT)/bin/passwd
	$(LN) -s ../usr/bin/passwd $(DESTROOT)/bin/passwd
	$(RM) $(DESTROOT)/usr/bin/chfn $(DESTROOT)/usr/bin/chsh
	$(LN) $(DESTROOT)/usr/bin/passwd $(DESTROOT)/usr/bin/chfn
	$(LN) $(DESTROOT)/usr/bin/passwd $(DESTROOT)/usr/bin/chsh

include $(GMAKERULES)
