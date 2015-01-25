PC := 0.4
PI := 0.4
export PC
export PI
export ROOTDIR := $(shell pwd)

TARGETS := all clean debug release verbose

.PHONY : $(TARGETS)

$(TARGETS) : 
	@ $(MAKE) --no-print-directory -C src $@
