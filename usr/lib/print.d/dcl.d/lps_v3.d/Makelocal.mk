# @(#)Makelocal.mk	4.3      ULTRIX 	10/16/90

# Makefile for ./dcl.d/lps_v3.d
#
#
# AUTHOR:	Adrian Thoms
# DATE:		28th February 1989
# Modification History:
#
# 01-Oct-1990 - Adrian Thoms
#	Added LPS_SEPARATE, LPS_PS_EXT and LPS_PUNCHED files to this directory
#	Of these only LPS_SEPARATE has been added to the library.
#	LPS_PS_EXT and LPS_PUNCHED are installed to /usr/examples/print/lps

include $(GMAKEVARS)

DCLDIR=usr/lib/lpdfilters
LPSEXDIR=usr/examples/print/lps

DESTLIST= $(DESTROOT)/$(DCLDIR) \
	$(DESTROOT)/usr/examples \
	$(DESTROOT)/usr/examples/print \
	$(DESTROOT)/${LPSEXDIR}

DCLLIB		=lps_v3.a

DCL_MODULES	= \
	LPS_DECMCSENCODING \
	LPS_EOJ \
	LPS_ERRORHANDLER \
	LPS_FINDFONT_ISOLATIN1_DECMCS_V40 \
	LPS_FLAGPAGE \
	LPS_FLUSHPAGES \
	LPS_ISOLATIN1ENCODING \
	LPS_JOBJOG \
	LPS_LOADDICT \
	LPS_MESSAGEPAGE \
	LPS_SEPARATE \
	LPS_SETCONTEXT \
	LPS_SETINPUTTRAY \
	LPS_SETNUMBERUP \
	LPS_SETOUTPUTTRAY \
	LPS_SETPAGELIMIT \
	LPS_SETPAGEORIENTATION \
	LPS_SETPAGESIZE \
	LPS_SETSHEETCOUNT \
	LPS_SETSHEETSIZE \
	LPS_SETSIDES \
	LPS_TRAILPAGE

POSTSCRIPT_EXAMPLES= \
	LPS_PS_EXT \
	LPS_PUNCHED

all:	myall

myall:	$(DCLLIB)

$(DCLLIB):	clean ${DCL_MODULES}
	cd ..; $(AR) qv _${MACHINE}.b/$@ ${DCL_MODULES}

install:
	$(INSTALL) -c -m 644 $(DCLLIB) $(DESTROOT)/$(DCLDIR)/$(DCLLIB)
	for i in ${POSTSCRIPT_EXAMPLES}; do \
		$(INSTALL) -c -m 644 ../$$i $(DESTROOT)/${LPSEXDIR}/$$i; \
	done

include $(GMAKERULES)
