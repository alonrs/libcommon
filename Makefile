LIB_DIR  ?= lib
BIN_DIR  ?= bin
TST_DIR  ?= tests
CC       ?= gcc
CLINK    ?= $(CC)
CFLAGS   := -std=gnu11 -Wall -g -I.
LDFLAGS  := -lpthread -lm

# Include all user-defined functions
include ./functions.mk

# Create rules for all object files
$(call createmodule,$(LIB_DIR),c,lib)
$(call createmodule,$(TST_DIR),c,tst) 

# Search for all objects, executables
LIB_OBJ:=$(patsubst $(LIB_DIR)/%.c,$(BIN_DIR)/%.o,$(wildcard $(LIB_DIR)/*.c))
TST_OBJ:=$(patsubst $(TST_DIR)/%.c,$(BIN_DIR)/%.o,$(wildcard $(TST_DIR)/*.c))
TST_EXE:=$(patsubst $(TST_DIR)/%.c,$(BIN_DIR)/%.exe,$(wildcard $(TST_DIR)/*.c))

release: $(BIN_DIR)/libcommon.a $(TST_EXE)
debug:   $(BIN_DIR)/libcommon.a $(TST_EXE)

$(BIN_DIR)/%.exe: $(BIN_DIR)/%.o $(BIN_DIR)/libcommon.a
	$(CLINK) $(CFLAGS) $+ -o $@ $(LDFLAGS)

$(BIN_DIR)/libcommon.a: $(LIB_OBJ)
	$(AR) -cq $@ $+

# Include submodule with rules to create objects
include $(BIN_DIR)/*.mk

# Target specific variables
release: CFLAGS += -O2 -DNDEBUG
debug:   CFLAGS += -O0

clean:
	rm -rf $(BIN_DIR)
