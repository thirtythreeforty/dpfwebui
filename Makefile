# Filename: Makefile
# Author:   oss@lucianoiam.com

examples: webui
	@(echo 'Run make wasm to include the AssemblyScript examples')

webui:
	@make -C examples/webgain
	@make -C examples/zcomp
	@make -C examples/xwave

wasm: webui
	@make -C examples/jitdrum
	@make -C examples/astone
	@make -C examples/hotswap

clean:
	@make clean -C examples/webgain
	@make clean -C examples/zcomp
	@make clean -C examples/xwave
	@make clean -C examples/jitdrum
	@make clean -C examples/astone
	@make clean -C examples/hotswap
	rm -rf build/*

all: examples

.PHONY: examples
