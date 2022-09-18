#	sys/dist/data.d/_mips.d/Makelocal.mk -
#		local makefile for ULTRIX (RISC) manufacturing data
#
#  "@(#)Makelocal.mk	4.4 (ULTRIX) 12/20/90"
#
# 	002	11-Oct-90
#	Updated for V4.2	overman
#
#	001	17-jul-1990	mdf
#	Updated for V4.1
#
#	000	02-mar-1989	ccb
#	New.

include $(GMAKEVARS)

TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

KFILES= UDT420.k UDT421.k UDX420.k UDC420.k
MIFILES= UDT420.mi UDT421.mi

install:
	@for i in $(KFILES) $(MIFILES); \
	do \
		echo "$(INSTALL) -c -m 644 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 644 ../$$i $(TODIR)/$$i; \
	done

include $(GMAKERULES)
