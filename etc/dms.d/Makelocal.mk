#  @(#)Makelocal.mk	4.2	ULTRIX  10/9/90

include $(GMAKEVARS)

DESTDIR=$(DESTROOT)/usr/diskless
DESTLIST=$(DESTROOT)/etc $(DESTDIR) $(DESTDIR)/dev

ENVS=crontab
SCRIPT=fscarv makconf make_client dms.i dms.a dmsinit

AOUTS=make_swap passwd_dlws tzone showbootpt

make_swap:	make_swap.o
make_swap.o:	make_swap.c

passwd_dlws:	passwd_dlws.o
passwd_dlws.o:	passwd_dlws.c

tzone:	tzone.o
tzone.o:	tzone.c

showbootpt:	showbootpt.o
showbootpt.o:	showbootpt.c

all:	$(AOUTS)

install: $(AOUTS)
	$(INSTALL) -c -m 700 ../dms $(DESTROOT)/etc/dms
	$(INSTALL) -c -m 700 -s make_swap $(DESTDIR)/make_swap
	$(INSTALL) -c -m 700 -s passwd_dlws $(DESTDIR)/passwd_dlws
	$(INSTALL) -c -m 755 -s tzone $(DESTDIR)/tzone
	$(INSTALL) -c -m 700 -s showbootpt $(DESTDIR)/showbootpt
	@for i in $(SCRIPT); do \
		$(ECHO) "$(INSTALL) -c -m 700 ../$$i $(DESTDIR)/$$i"; \
		$(INSTALL) -c -m 700 ../$$i $(DESTDIR)/$$i; \
	done;
	@for i in $(ENVS); do \
		$(ECHO) "$(INSTALL) -c -m 644 ../$$i $(DESTDIR)/$$i"; \
		$(INSTALL) -c -m 644 ../$$i $(DESTDIR)/$$i; \
	done;

include $(GMAKERULES)
