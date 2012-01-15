opcontrol --no-vmlinux
opcontrol --reset
opcontrol --setup --event=CPU_CLK_UNHALTED:6000
opcontrol --start

