# /*
# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: BSD-3-Clause
# */

#
# Root Makefile
# Arvind Rawat
#
#
TPMEXT_ROOT=..
BIN=$(TPMEXT_ROOT)/bin/debug
OBJ=$(TPMEXT_ROOT)/build

RMBIN=$(TPMEXT_ROOT)/bin/debug
RMOBJ=$(TPMEXT_ROOT)/build

BUILD_TARGETS=  makedirstructure \
				tpmextend-g

TARGETS=$(BUILD_TARGETS) clean

.PHONY: $(TARGETS)

all: $(BUILD_TARGETS)

makedirstructure:
	mkdir -p $(BIN)
	mkdir -p $(OBJ)

tpmextend-g:
	$(MAKE) debug=$(debug) -f tpmextend-g.mak

clean:
	rm -f $(RMBIN)/*
	rm -f $(RMOBJ)/*
