#	Makelocal.mk
#		sys/dist/setld.d/shell.d Makefile
#
#     "@(#)Makelocal.mk	4.4 (ULTRIX) 11/15/90"
#
#	000	03-mar-1989	ccb
#	New.
#
#	001	11-may-1989	ccb
#		remove setld.new
#
#	002	19-jun-1989	ccb
#		add dummy lint rule
#
#	003	10-sep-1990	ccb
#		add genufi, invsync, updmv. Installed to $(STL)
#
#	004	06-nov-1990	ccb
#		move etc/stl to /usr
#       005     15-nov-1990     jps
#               added updmerge
#
include $(GMAKEVARS)

STL=usr/etc/stl
DIST=usr/sys/dist

DESTLIST= $(DESTROOT)/etc $(DESTROOT)/$(STL) $(DESTROOT)/$(DIST) \
	$(DESTROOT)/usr/kits $(DESTROOT)/var/kits

DISTFILES= genra genrx gentapes kits newinv
ETCFILES= ready setld getrisroot
STLFILES= genufi invsync updmerge updmv

install:	$(DISTFILES) $(ETCFILES) $(STLFILES)
	@for i in $(DISTFILES); \
	do \
		$(ECHO) "$(INSTALL) -c -m 755 ../$$i $(DESTROOT)/$(DIST)/$$i"; \
		$(INSTALL) -c -m 755 ../$$i $(DESTROOT)/$(DIST)/$$i; \
	done
	@for i in $(ETCFILES); \
	do \
		$(ECHO) "$(INSTALL) -c -m 755 ../$$i $(DESTROOT)/etc/$$i"; \
		$(INSTALL) -c -m 755 ../$$i $(DESTROOT)/etc/$$i; \
	done
	@for i in $(STLFILES); \
	do \
		$(ECHO) "$(INSTALL) -c -m 755 ../$$i $(DESTROOT)/$(STL)/$$i"; \
		$(INSTALL) -c -m 755 ../$$i $(DESTROOT)/$(STL)/$$i; \
	done

lint:

include $(GMAKERULES)
