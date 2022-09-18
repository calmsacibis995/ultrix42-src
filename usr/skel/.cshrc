if ($?prompt) then
	set prompt = "csh> "
	set notify
	set history = 20
	alias pd pushd
	alias pop popd
	alias vt52 "set term = vt52"
	alias ti745 "set term = ti745 ; stty -tabs"
	alias ti785 "set term = ti745 ; stty -tabs"
	alias vt100 "set term = vt100"
endif
