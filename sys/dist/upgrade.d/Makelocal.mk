#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/usr/sys/dist $(DESTROOT)/usr/sys/dist/upgrade
UPGDIR= $(DESTROOT)/usr/sys/dist

SCRIPTS= genupgrade install_upgrade
UPGRADES=0 2 8 16 32 64

AOUT= makeupgrade
OBJS= makeupgrade.o

makeupgrade.o:	makeupgrade.c

install:	$(AOUTS)
	$(INSTALL) -c -m 755 ../genupgrade $(DESTROOT)/usr/sys/dist/genupgrade
	@for i in $(UPGRADES); \
	do \
		$(ECHO) "./makeupgrade $$i"; \
		./makeupgrade $$i; \
		$(ECHO) "$(INSTALL) -c -m 400 upgrade$$i $(UPGDIR)/upgrade$$i"; \
		$(INSTALL) -c -m 400 upgrade$$i $(UPGDIR)/upgrade$$i; \
	done
	ln $(UPGDIR)/upgrade0 $(UPGDIR)/upgrade.nolimit
	$(INSTALL) -c -m 700 ../install_upgrade $(DESTROOT)/etc/install_upgrade


include $(GMAKERULES)
