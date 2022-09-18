#	Makelocal.mk
#		sys/dist Makefile
#
#	000	27-jul-1990	ccb
#	New.
#
#	001	05-aug-1990	overman
#	changed to install into /usr/etc - per the request of submit review
#
#	SCCSID	"@(#)Makelocal.mk	1.3 (ULTRIX)	8/5/90"

include $(GMAKEVARS)

ETCDIR=$(DESTROOT)/usr/etc
SCAMPDIR= $(ETCDIR)/scampdir

DESTLIST= $(ETCDIR) $(SCAMPDIR)
SUBDIRS=lpr_info.d text_info.d

INSTALLFILES= LMF SSINS SSREM SSSIZ add_user backitup datetime \
	modemsetup printsetup rm_user scamp shut_down ssizes sysname \
	termsetup tty_scan tz_dst

install:	$(INSTALLFILES)
	@for i in $(INSTALLFILES); \
        do \
                $(ECHO) "$(INSTALL) -c -m 755 ../$$i $(SCAMPDIR)/$$i"; \
                $(INSTALL) -c -m 755 ../$$i $(SCAMPDIR)/$$i; \
        done
	(cd $(ETCDIR);ln -s scampdir/scamp scamp)

include $(GMAKERULES)
