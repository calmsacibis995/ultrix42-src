#  @(#)Makerules.mk	2.6  ULTRIX  7/27/89

# Each rule assume that the current directory
# is one of the _*.b subdirectories.

all:	$(SUBDIRS) $(ARFILE) $(AOUT) $(AOUTS)

install: $(DESTLIST) $(SUBDIRS)

sccsget: sccsget$(MACHINE)

sccsgetvax:
	@$(ECHO) \
		\( $(ECHO) $(CD) ..\;\
		 $(CD) ..\;\
		 $(ECHO) $(SCCSGETCMD) \;\
		 $(SCCSGETCMD) \) \;\
	| $(SHELL);
	@for SUBDIR in $(SUBDIRS) ; \
	do \
		$(ECHO) if [ "$$SUBDIR" != "nosubdirs" ] \;\
		then \
			$(ECHO) $(CD) ../$$SUBDIR\;\
			$(CD) ../$$SUBDIR\;\
			$(ECHO) $(SCCS) get $(MAKEFILES)\;\
			$(SCCS) get $(MAKEFILES)\;\
			$(ECHO) $(CD) _vax.b \;\
			$(CD) _vax.b \;\
			$(ECHO) $(MAKECMD) sccsgetvax\;\
			$(MAKECMD) sccsgetvax\;\
		else \
			true \;\
		fi | $(SHELL) ;\
	done

sccsgetmips:
	@$(ECHO) \
		\( $(ECHO) $(CD) ..\;\
		 $(CD) ..\;\
		 $(ECHO) $(SCCSGETCMD) \;\
		 $(SCCSGETCMD) \) \;\
	| $(SHELL);
	@for SUBDIR in $(SUBDIRS) ; \
	do \
		$(ECHO) if [ "$$SUBDIR" != "nosubdirs" ] \;\
		then \
			$(ECHO) $(CD) ../$$SUBDIR\;\
			$(CD) ../$$SUBDIR\;\
			$(ECHO) $(SCCS) get $(MAKEFILES)\;\
			$(SCCS) get $(MAKEFILES)\;\
			$(ECHO) $(CD) _mips.b \;\
			$(CD) _mips.b \;\
			$(ECHO) $(MAKECMD) sccsgetmips\;\
			$(MAKECMD) sccsgetmips\;\
		else \
			true \;\
		fi | $(SHELL) ;\
	done

sccsinfo: $(SUBDIRS)
	$(CD) ..; \
	$(SCCS) info

clobber: clobberlocal $(SUBDIRS)

clobberlocal:
	@$(ECHO) \
		\( $(ECHO) $(CD) ..\;\
		 $(CD) ..\;\
		 $(ECHO) $(SCCSCLEANCMD) \;\
		 $(SCCSCLEANCMD) \) \;\
	| $(SHELL)


cleanall:	cleanlocal $(SUBDIRS)

cleanlocal:
	-$(RM) ../_*.b/*

clean:	clean$(MACHINE)

cleanvax:
	-($(CD) ../_$(MACHINE).b; $(RM) *; true)
	@for SUBDIR in $(SUBDIRS) ; \
	do \
		$(ECHO) if [ "$$SUBDIR" != "nosubdirs" ] \;\
		then \
			$(ECHO) $(CD) ../$$SUBDIR/_vax.b \;\
			$(CD) ../$$SUBDIR/_vax.b \;\
			$(ECHO) $(MAKECMD) $@\;\
			$(MAKECMD) $@\;\
		else \
			true \;\
		fi | $(SHELL) ;\
	done

cleanmips:
	-($(CD) ../_$(MACHINE).b; $(RM) *; true)
	@for SUBDIR in $(SUBDIRS) ; \
	do \
		$(ECHO) if [ "$$SUBDIR" != "nosubdirs" ] \;\
		then \
			$(ECHO) $(CD) ../$$SUBDIR/_mips.b \;\
			$(CD) ../$$SUBDIR/_mips.b \;\
			$(ECHO) $(MAKECMD) cleanmips\;\
			$(MAKECMD) cleanmips\;\
		else \
			true \;\
		fi | $(SHELL) ;\
	done

#
# makes an a.out of the form "*.c -> *.o -> a.out"
#
$(AOUT):	$(OBJS) $(OBJSMISC)
	@$(DBGECHO) $(AOUT)
	@$(ECHO) if [ "$@" != "noaout" ] \;\
	then \
		$(ECHO) $(LDCMD) $(OBJS) $(OBJSMISC) $(LOADLIBES)\;\
		$(LDCMD) $(OBJS) $(OBJSMISC) $(LOADLIBES)\;\
	else \
		true \;\
	fi | $(SHELL)

#
# makes an archive of the form "*.c -> *.o -> ?.a"
#
$(ARFILE):	$(OBJS)
	@$(DBGECHO) $(ARFILE)
	@$(ECHO) if [ "$@" != "noarfile" ] \;\
	then \
		$(ECHO) $(AR) $@ $(ARFLAGS) $(OBJS) \;\
		$(AR) $(ARFLAGS) $@ $(OBJS) \;\
		$(ECHO) $(RANLIB) $@ \;\
		$(RANLIB) $@ \;\
	else \
		true \;\
	fi | $(SHELL)

#
# makes any number of a.outs of the form "a.c -> a.o -> a.out"
#
$(AOUTS):
	@$(DBGECHO) $(AOUTS)
	@$(ECHO) if [ "$@" != "noaouts" ] \;\
	then \
		$(ECHO) $(LDCMD) $? $(LOADLIBES)\;\
		$(LDCMD) $? $(LOADLIBES)\;\
	else \
		true \;\
	fi | $(SHELL)

#
# creates directories to be installed into
#
$(DESTLIST):
	@$(DBGECHO) $(DESTLIST)
	@$(ECHO) 'if [ "$(@)" != "nodestlist" ]; then \
		DIR=$(@); \
		while [ ! -d "$$DIR" ]; do \
			DIRS="$$DIR $$DIRS"; \
			DIR=`dirname "$$DIR"`; \
		done; \
		$(ECHO) "mkdir $$DIRS"; \
		mkdir $$DIRS; \
	else \
		true; \
	fi' | $(SHELL)

#
# make tree traversal
#
$(SUBDIRS):	FRC
	@$(DBGECHO) $(@)
	@$(ECHO) if [ "$@" != "nosubdirs" ] \;\
	then \
		$(CD) ../$(@)/_$(MACHINE).b \;\
 		$(ECHO) pwd=\`/bin/pwd\` \;\
		$(ECHO) $(MAKECMD) $(MAKERULE)\;\
		$(MAKECMD) $(MAKERULE)\;\
	else \
		true \;\
	fi | $(SHELL)

#
# Forced ReCompile
#
FRC:

.c.o:
	$(CCCMD) ../$<

.e.o:
	$(EC) $(EFLAGS) -c ../$<

.F.o:
	$(FC) $(FFLAGS) -c ../$<

.f.o:
	$(FC) $(FFLAGS) -c ../$<

.l.o:
	$(LEX) $(LFLAGS) ../$<
	$(CCCMD) lex.yy.c
	-$(RM) lex.yy.c
	$(MV) lex.yy.o $@

.p.o:
	$(PC) $(PFLAGS) -c ../$<

.r.o:
	$(RC) $(RFLAGS) -c ../$<

.s.o:
	$(CCCMD) ../$<

.y.o:
	$(YACC) $(YFLAGS) ../$<
	$(CCCMD) y.tab.c
	-$(RM) y.tab.c
	$(MV) y.tab.o $@
#
# don't "sccs get" for .DEFAULT:
# may interfere with other simultaneous $(MACHINE) build
#
.DEFAULT:
	@$(ECHO) "No such make rule:" $(@)
