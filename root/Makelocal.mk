#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
# This Makefiles installs the . files that live in the root of the 
# distribution. 
#  
include $(GMAKEVARS)

# DESTROOT is the directory below which the distribution is built.

DESTROOT=

FILES= ../.login ../.cshrc ../.profile ../.rhosts 
FILES_CHMOD= .login .cshrc .profile .rhosts 

VERSION= 1.0

install: ${FILES}
	cp ${FILES} ${DESTROOT}/.
	( cd ${DESTROOT}/.; chmod 744 ${FILES_CHMOD} )

save_version:
	@sccs get -s SCCS
	@sccs what ${FILES} ../Makelocal.mk | awk '$$2 != "" {print $$1, $$2}' > SCCS/version_${VERSION}

restore_version:
	@echo ../library/root:
	@for i in ${FILES} ../Makelocal.mk; do \
		echo "Restore" `grep $$i SCCS/version_${VERSION}`; \
		sccs get -s -r`grep $$i SCCS/version_${VERSION} | awk '{print $$2, $$1}'`; \
	done

show_current:
	@echo ../library/root:
	@sccs what ${FILES} ../Makelocal.mk | awk '$$2 != "" {printf "%s  %s\t", $$1, $$2}'
	@echo 


include $(GMAKERULES)
