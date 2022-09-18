#	Makelocal.mk -
#		sys/dist/setld.d Makefile
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
#	000	03-mar-1989	ccb
#	New.
#
#	001	19-apr-1989	ccb
#		added h.d and lib.d dirs for update installation
#

include $(GMAKEVARS)

SUBDIRS= h.d lib.d c.d shell.d

lint:	$(SUBDIRS) lintlocal

lintlocal:

include $(GMAKERULES)
