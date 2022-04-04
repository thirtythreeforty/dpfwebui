# Filename: Makefile
# Author:   oss@lucianoiam.com

examples:
	@make -C examples/webgain

# WAMR depends on building LLVM from source and takes a very long while
wasm:
	@make -C examples/jitdrum
	@make -C examples/astone
	@make -C examples/hotswap

clean:
	@make clean -C examples/webgain
	@make clean -C examples/jitdrum
	@make clean -C examples/astone
	@make clean -C examples/hotswap
	rm -rf build/*

all: examples

.PHONY: examples
