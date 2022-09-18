#	Makelocal.mk -
#		sys/dist/etc.d/fstab.d Makefile
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
#	000	02-mar-1989	ccb
#	New. Installs default (RA60) fstab to each machine
#	after calling rule to install architecture specific files

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/etc

VAXFILES= fstab.hp fstab.hp400m fstab.ra60 fstab.ra80 fstab.ra81 \
	fstab.rb80 fstab.rc25 fstab.rk07 fstab.rm03 fstab.rm05 fstab.rm80 \
	fstab.rp06 fstab.rp07 fstab.up fstab.up160m fstab.up300m

MIPSFILES=

install:	fstab_$(MACHINE)
	$(INSTALL) -c -m 644 ../fstab.ra60 $(DESTROOT)/etc/fstab

fstab_vax:
	@for i in $(VAXFILES); \
	do \
		echo "$(INSTALL) ../$$i $(DESTROOT)/etc/$$i"; \
		$(INSTALL) -c -m 644 ../$$i $(DESTROOT)/etc/$$i; \
	done

fstab_mips:

include $(GMAKERULES)
