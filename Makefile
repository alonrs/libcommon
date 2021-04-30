LIB_DIR  ?= lib
BIN_DIR  ?= bin
TST_DIR  ?= tests
CC       ?= gcc
CLINK    ?= $(CC)
CFLAGS   := -std=gnu11 -Wall -g -I.
LDFLAGS  := -lpthread -lm

# Create rules for every C file in $(1) in $(BIN_DIR)/objects_($2).mk
createmodule = $(shell $(CC) $(CFLAGS) -MM $(1)/*.c |               \
    sed -E "s@^(.*):@$(BIN_DIR)/\1:@g"              |               \
    awk 'NR>1 && /:/ {                                              \
             printf "\t$$(CC) $$(CFLAGS) -o $$@ -c %s\n%s\n",       \
                    "$$(patsubst $(BIN_DIR)/%.o,$(1)/%.c,$$@)", $$0 \
         } NR==1 || !/:/ {                                          \
             print $$0                                              \
         } END {                                                    \
         	 printf "\t$$(CC) $$(CFLAGS) -c -o $$@ %s\n",           \
                    "$$(patsubst $(BIN_DIR)/%.o,$(1)/%.c,$$@)"      \
         }' > $(BIN_DIR)/objects_$(2).mk)

# Create bin directory and make submodule
ifeq "$(wildcard $(BIN_DIR) )" ""
    $(info Creating makefile for all object files...)
    $(shell mkdir -p $(BIN_DIR))
    $(call createmodule,$(LIB_DIR),lib)
    $(call createmodule,$(TST_DIR),tst) 
endif

# Search for all objects, executables
LIB_SRC:=$(wildcard $(LIB_DIR)/*.c)
LIB_OBJ:=$(patsubst $(LIB_DIR)/%.c,$(BIN_DIR)/%.o,$(wildcard $(LIB_DIR)/*.c))
TST_SRC:=$(wildcard $(TST_DIR)/*.c)
TST_OBJ:=$(patsubst $(TST_DIR)/%.c,$(BIN_DIR)/%.o,$(wildcard $(TST_DIR)/*.c))
TST_EXE:=$(patsubst $(TST_DIR)/%.c,$(BIN_DIR)/%.exe,$(wildcard $(TST_DIR)/*.c))

release: $(BIN_DIR)/libcommon.a $(TST_EXE)
debug:   $(BIN_DIR)/libcommon.a $(TST_EXE)

$(BIN_DIR)/%.exe: $(BIN_DIR)/%.o $(BIN_DIR)/libcommon.a
	$(CLINK) $(CFLAGS) $+ -o $@ $(LDFLAGS)

$(BIN_DIR)/libcommon.a: $(LIB_OBJ)
	ar -cvq $@ $+

# Include submodule with rules to create objects
include $(BIN_DIR)/objects_lib.mk
include $(BIN_DIR)/objects_tst.mk

# Target specific variables
release: CFLAGS += -O2 -DNDEBUG
debug:   CFLAGS += -O0

clean:
	rm -rf $(BIN_DIR)
