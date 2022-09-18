stty dec new cr0
tset -I -Q
umask 022
setenv MAIL /usr/spool/mail/$USER
set mail=$MAIL
set path=($HOME/bin /usr/ucb /bin /usr/bin /usr/local /usr/new /usr/hosts .)
set prompt="`hostname`> "
setenv EXINIT 'set redraw wm=8'
biff n
date
