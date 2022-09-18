# @(#)Makelocal.mk	4.1      ULTRIX	7/2/90

# Makefile -- for ansi to PostScript translator
#
# AUTHOR:       Andy Roach
# DATE:         May 4th 1989
#
# Modification history
#
# 13 Nov 1989	Adrian Thoms
#	Added missing dependencies, new file u_dispose_prologue.c
#	-D compile rule for xlc_main.o
#	Installation of preamble.ps
#
# 14 May 1989	Giles Atkinson
#	Compile cpvars.c with -w flag to suppress warnings

include $(GMAKEVARS)

XLDIR		=usr/lib/lpdfilters
DESTLIST	=$(DESTROOT)/$(XLDIR)

HEADERS		= cacfff.hc caexit.hc caglobal.hc caglobal.vc \
		camac.lib_hc capdl.hc cpast.hc cpast.vc cpexit.hc \
		cpglobal.hc cpglobal.vc cpmac.lib_hc cpparse.hc \
		cpsixel.hc cpsys.hc cpsys.vc cptok.tdef cptok.thc \
		dbug.h dumpu.hc pdl_pdli.hc portab.h portab_gen.h \
		xlate.h \
		xlc_codegen.hc xlc_codegen.vc xlc_debug.hc xlc_debug.vc \
		xlc_font_dictionary.hc xlc_font_dictionary.vc \
		xlc_font_init.hc xlc_font_init.vc \
		xlc_font_metrics.hc xlc_font_metrics.vc \
		xlc_graph.hc xlc_graph.vc xlc_iface.hc \
		xlc_init.hc xlc_main.hc xlc_main.vc \
		xlc_ps.hc xlm_codegen.h xlm_io.hc \
		xlc_main.vc xlc_main.vc

OBJS            = cacfff.o caclib.o cacsys.o cadvec.o caexit.o \
		cafont_cfont.o cafont_forms.o cafont_load.o \
		cafont_sel.o cafont_sgr.o cagraph.o cainit.o \
		cajfy.o camode.o caparse.o capctrl.o caspac.o \
		castate.o catabs.o catmgt.o cavars.o cpbuf_mgt.o \
		cpexit.o cpinput.o cpparse.o cpsixel.o cpspoint.o \
		cpstartup.o cpvars.o dumpu.o pdl_pdli.o xlc_iface.o \
		xlc_codegen.o xlc_dll.o xlc_font_dictionary.o \
		xlc_font_init.o xlc_font_metrics.o xlc_graph.o \
		xlc_init.o xlc_main.o xlc_vars.o u_main.o \
		u_dispose_prologue.o

LIBLP=../../../lib.d/_$(MACHINE).b/liblp.a
LOADLIBES=$(LIBLP)

CINCLUDES	= -I. -I.. -I$(SRCROOT)/usr/include -I../../../h.d
AOUT=ansi_ps

$(OBJS): $(HEADERS)


install:
		install -c -s ansi_ps ${DESTROOT}/${XLDIR}/ansi_ps
		install -c -m 644 ../preamble.ps ${DESTROOT}/${XLDIR}/preamble.ps

# Without -w the machine generated code included by cpvars.c produces
# over a hundred warnings - the code genereator should be fixed.

cpvars.o: cpvars.c
		${CCCMD} -w ../$<

cacfff.o: cacfff.c
caclib.o: caclib.c
cacsys.o: cacsys.c
cadvec.o: cadvec.c
caexit.o: caexit.c
cafont_cfont.o: cafont_cfont.c
cafont_forms.o: cafont_forms.c
cafont_load.o: cafont_load.c
cafont_sel.o: cafont_sel.c
cafont_sgr.o: cafont_sgr.c
cagraph.o: cagraph.c
cainit.o: cainit.c
cajfy.o: cajfy.c
camode.o: camode.c
caparse.o: caparse.c
capctrl.o: capctrl.c
caspac.o: caspac.c
castate.o: castate.c
catabs.o: catabs.c
catmgt.o: catmgt.c
cavars.o: cavars.c
cpbuf_mgt.o: cpbuf_mgt.c
cpexit.o: cpexit.c
cpinput.o: cpinput.c
cpparse.o: cpparse.c
cpsixel.o: cpsixel.c
cpspoint.o: cpspoint.c
cpstartup.o: cpstartup.c
dumpu.o: dumpu.c
pdl_pdli.o: pdl_pdli.c
xlc_codegen.o: xlc_codegen.c
xlc_dll.o: xlc_dll.c
xlc_font_dictionary.o: xlc_font_dictionary.c
xlc_font_init.o: xlc_font_init.c
xlc_font_metrics.o: xlc_font_metrics.c
xlc_graph.o: xlc_graph.c
xlc_iface.o: xlc_iface.c
xlc_init.o: xlc_init.c

xlc_main.o: xlc_main.c
	${CCCMD} -Ddispose_prologue=u_dispose_prologue ../$<

xlc_vars.o: xlc_vars.c
u_main.o: u_main.c
u_dispose_prologue.o: u_dispose_prologue.c

include $(GMAKERULES)
