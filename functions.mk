# Mush use base in order to use "source"
SHELL:=/bin/bash

# Include common shell functions
mkfile_path :=$(abspath $(lastword $(MAKEFILE_LIST)))
current_dir :=$(dir $(mkfile_path))
shell_funcs :=$(current_dir)shell-functions.sh

# Create GNU rules for every source in $(1). Saves output to
# $(BIN_DIR)/objects_($2).mk
# @param $1 Files
# @param $2 Suffix for generated mk file
createmodule_c  =$(shell source $(shell_funcs) && \
                   echo "$(1)" | \
                   shell_createmodule "CC" "CFLAGS" \
				                      "$(2)" "$(CC)" "$(CFLAGS)")
createmodule_cpp=$(shell source $(shell_funcs) && \
                   echo "$(1)" | \
                   shell_createmodule "CXX" "CXXFLAGS"\
				                      $(2) "$(CXX)" "$(CXXFLAGS)")

# For each C file with path format $(1)/xxxx.c, create a path string 
# with the format $(2)/xxxx.o. Filter out files with the basename prefix $(3)-
# @param $1 Input directory
# @param $2 Output directory
# @param $3 Extension of source files
# @param $4 File prefix to filter out (optional)
collectobjects = $(filter-out $(2)/$(4)-%.o,     \
                       $(patsubst $(1)/%.$(3),   \
                       $(2)/%.o,                 \
                       $(wildcard $(1)/*.$(3))))


# For each C file with path format $(1)/$(3)xxxx.c, create a path string
# with the format $(2)/$(3)xxxx.exe.
# @param $1 Input directory
# @param $2 Output directory
# @param $3 Extension of source files
# @param $4 File prefix (optional)
collectexecutables = $(patsubst $(1)/$(4)%.$(3),   \
                       $(2)/$(4)%.exe,             \
                       $(wildcard $(1)/$(4)*.$(3)))


# Check whether the CPU supports $(1); if so, add the flag $(2) to AUTOFLAGS
# @param $1 CPU extension to check for
# @param $2 Flag to insert into AUTOFLAGS
cpucheck = $(shell cat /proc/cpuinfo | grep " $(1) " | awk  \
    "END{if (NR>1) print \"AUTOFLAGS+=$(2)\";}"             \
    >> $(BIN_DIR)/config.mk)

# Create bin directory, configure AUTOFLAGS (if empty!)
ifeq "$(wildcard $(BIN_DIR) )" ""
    $(shell mkdir $(BIN_DIR))
ifeq "$(AUTOFLAGS)" ""
    $(call cpucheck,avx,"-mavx")
    $(call cpucheck,avx2,"-mavx2")
    $(call cpucheck,fma,"-mfma")
    $(call cpucheck,popcnt,"-mpopcnt")
endif
endif
