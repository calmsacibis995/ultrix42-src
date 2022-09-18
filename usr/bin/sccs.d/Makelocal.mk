# 	@(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
# Make file to recreate all of sccs
#
include $(GMAKEVARS)
SUBDIRS=hdr lib libPW cmd help

CMDS=admin bdiff comb delta get prs rmchg unget val vc what
HELPS=ad bd cb cm cmds co de default ge he prs rc sc un ut vc

TESTDIR=../cmd/_$(MACHINE).b
LIB = /usr/lib
INSDIR=/usr/bin
FEINSD=/usr/ucb
HELPDIR=../help
PUB_HELPLIB=$(LIB)/sccs.help
OWNER = root
GROUP = staff
FEOWN = sccs
FEGRP = staff

# sccs is put in build environ just in case we happen to be chrooted
# in build environ and we want to do sccs cmds.  Current build procedure
# does the initial sccsget rules while NOT in chroot
pretools tools1: all
pretools tools1: $(DESTROOT)/usr $(DESTROOT)$(INSDIR) $(DESTROOT)$(FEINSD) \
		$(DESTROOT)$(LIB) $(DESTROOT)$(PUB_HELPLIB)
pretools tools1:
	for i in $(CMDS); do \
		install -c -s -o $(OWNER) -g $(GROUP) -m 755 \
			$(TESTDIR)/$$i $(DESTROOT)$(INSDIR)/$$i ; \
	done
	-rm -f $(DESTROOT)$(INSDIR)/cdc
	ln $(DESTROOT)$(INSDIR)/rmchg $(DESTROOT)$(INSDIR)/cdc
	-rm -f $(DESTROOT)$(INSDIR)/rmdel
	ln $(DESTROOT)$(INSDIR)/rmchg $(DESTROOT)$(INSDIR)/rmdel
	-rm -f $(DESTROOT)$(INSDIR)/sact
	ln $(DESTROOT)$(INSDIR)/unget $(DESTROOT)$(INSDIR)/sact
	install -c -o $(OWNER) -g $(GROUP) -m 755 \
			$(TESTDIR)/../sccsdiff.sh $(DESTROOT)$(INSDIR)/sccsdiff
	install -c -s -o $(FEOWN) -g $(FEGRP) -m 4555 \
			$(TESTDIR)/sccs $(DESTROOT)$(FEINSD)/sccs
# pretools tools1 doesnt do the sccs help cmds


install: $(DESTROOT)/usr $(DESTROOT)$(INSDIR) $(DESTROOT)$(FEINSD) \
		$(DESTROOT)$(LIB) $(DESTROOT)$(PUB_HELPLIB)
	for i in $(CMDS); do \
		install -c -s -o $(OWNER) -g $(GROUP) -m 755 \
			$(TESTDIR)/$$i $(DESTROOT)$(INSDIR)/$$i ; \
	done
	-rm -f $(DESTROOT)$(INSDIR)/cdc
	ln $(DESTROOT)$(INSDIR)/rmchg $(DESTROOT)$(INSDIR)/cdc
	-rm -f $(DESTROOT)$(INSDIR)/rmdel
	ln $(DESTROOT)$(INSDIR)/rmchg $(DESTROOT)$(INSDIR)/rmdel
	-rm -f $(DESTROOT)$(INSDIR)/sact
	ln $(DESTROOT)$(INSDIR)/unget $(DESTROOT)$(INSDIR)/sact
	for i in $(HELPS); do \
		install -c -o $(OWNER) -g $(GROUP) -m 644 \
			$(HELPDIR)/$$i $(DESTROOT)$(PUB_HELPLIB)/$$i ; \
	done
	install -c -o $(OWNER) -g $(GROUP) -m 755 \
			$(TESTDIR)/../sccsdiff.sh $(DESTROOT)$(INSDIR)/sccsdiff
	install -c -s -o $(OWNER) -g $(GROUP) -m 755 \
			$(TESTDIR)/help $(DESTROOT)$(INSDIR)/sccshelp
	install -c -s -o $(FEOWN) -g $(FEGRP) -m 4555 \
			$(TESTDIR)/sccs $(DESTROOT)$(FEINSD)/sccs

$(DESTROOT)/usr $(DESTROOT)$(INSDIR) $(DESTROOT)$(FEINSD) \
  $(DESTROOT)$(LIB) $(DESTROOT)$(PUB_HELPLIB):
		mkdir $@
		chmod 755 $@
		-/etc/chown root $@
		-chgrp system $@

include $(GMAKERULES)
