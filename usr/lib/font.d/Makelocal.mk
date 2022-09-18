#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

#  This is the makefile for nroff font files and (via SUBDIRS) for
#	Adobe Font Metrics files used in formatting PostScript files
#	for printing and converting.
#
#	DO NOT submit files listed in "FILES" to SCCS!
#	DO NOT add files listed in FILES to clean rule!
#	These files are binary and SCCS is not yet ready for them.
#
SUBDIRS= metrics
FONTLIBDIR = $(DESTROOT)/usr/lib/font
CDABINDIR = /usr/bin
FILES = fontlist ftB ftBC ftC ftCE ftCI ftCK ftCW ftFD ftG \
	ftGI ftGM ftGR ftH ftHI ftHM ftI ftI.bak ftL ftLI ftPA \
	ftPB ftPI ftR ftS ftSB ftSI ftSM ftTB ftTI ftTR ftTS \
	ftUD ftap ftbb ftbi ftbk ftbr ftcb ftch ftci \
	ftcl ftco ftcr ftct ftcy ftdb ftdi ftdr ftfx ftgb ftgf \
	ftgi ftgk ftgr fthb fthn ftm ftmb ftmi ftmn ftmr ftnb \
	ftni ftnr ftob ftoe ftoi ftor ftpb ftpp ftsb ftsc ftsg \
	ftsh ftsi ftsl ftsr fttB fttI fttR fttS fttb ftti fttr \
	ftts ftwp ftxx ftch.old

$(FONTLIBDIR):
	@-if [ ! -d $(FONTLIBDIR) ]; then \
		mkdir $(FONTLIBDIR); \
	else true; \
	fi
	
install: $(FONTLIBDIR)
	@for i in $(FILES); do \
		$(ECHO) "$(INSTALL) -c ../$$i $(FONTLIBDIR)/$$i"; \
		$(INSTALL) -c ../$$i $(FONTLIBDIR)/$$i; \
	done

include $(GMAKERULES)
