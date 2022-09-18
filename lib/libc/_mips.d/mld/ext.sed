s/[	]/ /g
s/.*://
t clear2
: clear2

s/^.*exportdefine/#define/
t end

s/^export/extern/
t foo
d
:foo
s/;.*/;/
s/ *=.*/;/
t clear1
: clear1

s/(/(/
t proc

s@\[[^\[]*\]@\[\]@g
b end

: proc
s/(\(.*\))/(\/\* \1 \*\/);/

: end
b
