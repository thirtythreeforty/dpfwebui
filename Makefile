# Filename: Makefile
# Author:   oss@lucianoiam.com

examples:
	@make -C examples/jitdrum
	@make -C examples/webgain
	@make -C examples/astone
	@make -C examples/hotswap

clean:
	rm -rf bin
	rm -rf build

all: examples

.PHONY: examples
