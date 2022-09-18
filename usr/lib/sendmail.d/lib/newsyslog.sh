# @(#)newsyslog.sh	4.2	(ULTRIX)	1/25/91
cd /usr/spool/mqueue
/bin/rm syslog.7
/bin/mv syslog.6  syslog.7
/bin/mv syslog.5  syslog.6
/bin/mv syslog.4  syslog.5
/bin/mv syslog.3  syslog.4
/bin/mv syslog.2  syslog.3
/bin/mv syslog.1  syslog.2
/bin/mv syslog.0  syslog.1
/bin/mv syslog    syslog.0
/bin/cp /dev/null syslog
/bin/chmod 644    syslog
/etc/chown daemon syslog
/bin/chgrp daemon syslog
/bin/kill -1 `/bin/cat /etc/syslog.pid`
