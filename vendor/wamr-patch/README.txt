This directory contains a modified implementation of the WAMR C API that avoids
the usage of a wasm_engine_t singleton.

This alone is not enough for safe runtime embedding in a plugin environment,
WAMR must be also built with WAMR_DISABLE_HW_BOUND_CHECK=1.

See Makefile.plugins.mk for further details.
