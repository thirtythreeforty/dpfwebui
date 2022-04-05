# Filename: Makefile
# Author:   oss@lucianoiam.com

examples: webui
	@(echo '\033[0;33mHey!\033[0m run make wasm to include the AssemblyScript examples')

webui:
	@make -C examples/webgain

wasm: webui
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
