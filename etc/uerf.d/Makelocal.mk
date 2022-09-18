# @(#)Makelocal.mk	1.1	(ULTRIX)	11/14/89
#
# This Makelocal.mk file is to stop the build from printing error
# messages when it gets to this directory.
# 
include $(GMAKEVARS)

SUBDIRS = db src

include $(GMAKERULES)
