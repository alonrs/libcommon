# Create rules for every C file in $(1) in $(BIN_DIR)/objects_($2).mk
# @param $1 Directory to search C files in
# @param $2 Suffix for generated mk file
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

# Check whether the CPU supports $(1); if so, add the flag $(2) to CFLAGS
# @param $1 CPU extension to check for
# @param $2 Flag to insert into CFLAGS
cpucheck = $(shell cat /proc/cpuinfo | grep " $(1) " | awk  \
    "END{if (NR>1) print \"CFLAGS+=$(2)\";}"                \
    >> $(BIN_DIR)/config.mk)

# Create bin directory, configure common flags
ifeq "$(wildcard $(BIN_DIR) )" ""
    $(shell mkdir $(BIN_DIR))
    $(call cpucheck,avx,"-mavx")
    $(call cpucheck,avx2,"-mavx2")
    $(call cpucheck,fma,"-mfma")
    $(call cpucheck,popcnt,"-mpopcnt")
endif