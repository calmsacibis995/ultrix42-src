#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
# There is currently no provision for local mods (reorder) or
# user mods ($TERMCAP).  These will be coming eventually.
#
# PARTS=	header \
# 	adds annarbor beehive cdc concept datamedia dec diablo general \
# 	hardcopy hazeltine heath homebrew hp ibm lsi microterm misc \
# 	pc perkinelmer print special \
# 	tektronix teleray teletype televideo ti visual \
# 	trailer
# TERMINFO points to our install location ($DESTROOT)

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib/terminfo $(DESTROOT)/usr/src/usr.lib/terminfo

PART_A = ../adds.ti ../annarbor.ti ../beehive.ti \
	../cdc.ti ../concept.ti ../datamedia.ti \
	../dec.ti ../diablo.ti ../general.ti \
	../hardcopy.ti ../hazeltine.ti ../heath.ti \
	../homebrew.ti ../hp.ti ../ibm.ti ../lsi.ti \
	../microterm.ti ../misc.ti ../newvt.ti \
	../pc.ti ../perkinelm.ti ../print.ti \
	../special.ti ../tektronix.ti ../teleray.ti \
	../teletype.ti ../televideo.ti ../ti.ti \
	../virtual.ti ../visual.ti ../xterm.ti
PARTS=	../header $(PART_A) ../trailer

COMPILE=../../screen/_$(MACHINE).b/tic

all:	terminfo.src

install: 
	TERMINFO=$(DESTROOT)/usr/lib/terminfo $(COMPILE) -v terminfo.src
	$(CP) $(PARTS) $(DESTROOT)/usr/src/usr.lib/terminfo

terminfo.src:	$(PARTS)
	cat $(PARTS) > terminfo.src

include $(GMAKERULES)
