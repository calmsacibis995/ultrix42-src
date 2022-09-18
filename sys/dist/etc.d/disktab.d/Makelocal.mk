#	Makelocal.mk -
#		sys/dist/etc.d/disktab.d Makefile
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
#	000	02-mar-1989	ccb
#	New.
#
#	001	14-Jun-1989	map (Mark Parenti)
#	Change install rule because disktab is no longer machine-specific.
#

include $(GMAKEVARS)

install:
	$(INSTALL) -c -m 644 ../disktab $(DESTROOT)/etc/disktab

include $(GMAKERULES)
