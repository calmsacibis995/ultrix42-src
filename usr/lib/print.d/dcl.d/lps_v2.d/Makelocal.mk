# @(#)Makelocal.mk	4.3      ULTRIX 	10/16/90

# Makefile for dcl.d/lps_v2.d
#
#
# AUTHOR:	Adrian Thoms
# DATE:		28th February 1989
#
# Modification History:
#
# 01-Oct-90 - Adrian Thoms (thoms@wessex)
#	Added LPS_SEPARATE module to library
#
# 09-jan-90 -- thoms
#		Changed name of lib from lps40.a to lps_v2.a

include $(GMAKEVARS)

DCLDIR=usr/lib/lpdfilters

DESTLIST= $(DESTROOT)/$(DCLDIR)

DCLLIB		=lps_v2.a

DCL_MODULES	= \
	LPS_EOJ \
	LPS_FLAGPAGE \
	LPS_FLUSHPAGES \
	LPS_JOBJOG \
	LPS_LOADDICT \
	LPS_MESSAGEPAGE \
	LPS_SEPARATE \
	LPS_SETCONTEXT \
	LPS_SETNUMBERUP \

	LPS_SETPAGELIMIT \
	LPS_SETPAGEORIENTATION \
	LPS_SETPAGESIZE \
	LPS_SETSHEETCOUNT \
	LPS_SETSHEETLIMIT \
	LPS_SETSHEETSIZE \
	LPS_TRAILPAGE \
	LPS_DECMCSENCODING \
	LPS_ERRORHANDLER \
	LPS_FINDFONT_ISOLATIN1_DECMCS_V40 \
	LPS_SETINPUTTRAY \
	LPS_SETOUTPUTTRAY

all:	myall

myall:	$(DCLLIB)

$(DCLLIB):	clean ${DCL_MODULES}
	cd ..; $(AR) qv _${MACHINE}.b/$@ ${DCL_MODULES}

install:
	$(INSTALL) -c -m 644 $(DCLLIB) $(DESTROOT)/$(DCLDIR)/$(DCLLIB)
	-rm -f $(DESTROOT)/$(DCLDIR)/lps40.a
	ln $(DESTROOT)/$(DCLDIR)/$(DCLLIB) $(DESTROOT)/$(DCLDIR)/lps40.a

include $(GMAKERULES)
