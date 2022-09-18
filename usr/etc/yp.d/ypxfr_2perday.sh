#! /bin/sh
#
#	@(#)ypxfr_2perday.sh	4.1	ULTRIX	7/2/90
#
# @(#)ypxfr_2perday.sh 1.1 86/02/05 Copyr 1985 Sun Microsystems, Inc.  
# @(#)ypxfr_2perday.sh	2.1 86/04/16 NFSSRC
#
# ypxfr_2perday.sh - Do twice-daily yp map check/updates
#

# set -xv
/etc/yp/ypxfr hosts.byname
/etc/yp/ypxfr hosts.byaddr
/etc/yp/ypxfr ethers.byaddr
/etc/yp/ypxfr ethers.byname
/etc/yp/ypxfr netgroup
/etc/yp/ypxfr netgroup.byuser
/etc/yp/ypxfr netgroup.byhost
/etc/yp/ypxfr mail.aliases 
