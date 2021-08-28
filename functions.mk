# Create rules for every $(2) file in $(1) in $(BIN_DIR)/objects_($3).mk
# @param $1 Directory to search C files in
# @param $2 Extention of source files
# @param $2 Suffix for generated mk file
createmodule = $(shell $(CC) $(CFLAGS) -MM $(1)/*.$(2) |               \
    sed -E "s@^(.*):@$(BIN_DIR)/\1:@g"                 |               \
    awk 'NR>1 && /:/ {                                                 \
             printf "\t$$(CC) $$(CFLAGS) -o $$@ -c %s\n%s\n",          \
                    "$$(patsubst $(BIN_DIR)/%.o,$(1)/%.$(2),$$@)", $$0 \
         } NR==1 || !/:/ {                                             \
             print $$0                                                 \
         } END {                                                       \
             printf "\t$$(CC) $$(CFLAGS) -c -o $$@ %s\n",              \
                    "$$(patsubst $(BIN_DIR)/%.o,$(1)/%.$(2),$$@)"      \
         }' > $(BIN_DIR)/objects_$(3).mk)


# For each C file with path format $(1)/xxxx.c, create a path string 
# with the format $(2)/xxxx.o. Filter out files with the basename prefix $(3)-
# @param $1 Input directory
# @param $2 Output directory
# @param $3 File prefix to filter out (optional)
collectobjects = $(filter-out $(2)/$(3)-%.o, \
                       $(patsubst $(1)/%.c,  \
                       $(2)/%.o,             \
                       $(wildcard $(1)/*.c)))


# For each C file with path format $(1)/$(3)xxxx.c, create a path string
# with the format $(2)/$(3)xxxx.exe.
# @param $1 Input directory
# @param $2 Output directory
# @param $3 File prefix (optional)
collectexecutables = $(patsubst $(1)/$(3)%.c, \
                       $(2)/$(3)%.exe,        \
                       $(wildcard $(1)/$(3)*.c))


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
