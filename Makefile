# Filename: Makefile
# Author:   oss@lucianoiam.com

examples:
	@make -C examples/webgain
	@make -C examples/jitdrum
	@make -C examples/astone

clean:
	rm -rf bin
	rm -rf build
	rm -rf examples/jitdrum/dsp/build
	rm -rf examples/astone/dsp/build

all: examples

.PHONY: examples
