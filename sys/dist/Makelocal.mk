#	Makelocal.mk
#		sys/dist Makefile
#
#	000	02-mar-1989	ccb
#	New.
#	
#	001	20-feb-1991	overman
#	Added fis.d to the list of SUBDIRS
#
#  @(#)Makelocal.mk	4.4	ULTRIX	2/28/91

include $(GMAKEVARS)

SUBDIRS= data.d dev.d doconfig.d etc.d fis.d hosts.d install.d mfg.d \
	scps.d setld.d upgrade.d scamp.d

include $(GMAKERULES)
