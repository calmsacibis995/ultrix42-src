#	Makelocal.mk
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
#	001	15-mar-1989	map (Mark A. Parenti)
#	Add the DESTLIST rule.  Because the lower level Makelocal.mk files
#	have dependencies on their install rules, the dependencies were 
#	executed before the DESTLIST rule in those files. As a result,
#	the output directory did not exist when the install was executed.
#	By moving the DESTLIST rule here, we create the directories
#	before the install rule in the lower level files is executed.
#

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/dev $(DESTROOT)/usr/diskless/dev

SUBDIRS= _$(MACHINE).d

include $(GMAKERULES)


