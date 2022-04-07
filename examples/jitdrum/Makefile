#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_jitdrum

# --------------------------------------------------------------
# Project version for generating unique symbol names

HIPHOP_PROJECT_VERSION = 1

# --------------------------------------------------------------
# Enable AssemblyScript by setting DSP files location

HIPHOP_AS_DSP_PATH = dsp

# --------------------------------------------------------------
# Enable Web UI by setting web files location

HIPHOP_WEB_UI_PATH = ui

# --------------------------------------------------------------
# Enable Web UI control from devices in the local network

HIPHOP_NETWORK_UI = true

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
    JITDrumExamplePlugin.cpp

FILES_UI  = \
    JITDrumExampleUI.cpp

# --------------------------------------------------------------
# Do some magic

DPF_TARGET_DIR = ../../bin
DPF_BUILD_DIR = ../../build/jitdrum/1/2

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

TARGETS += lv2_sep vst vst3

all: $(TARGETS) $(HIPHOP_TARGET)

# --------------------------------------------------------------
