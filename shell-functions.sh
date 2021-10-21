#!/bin/bash

# Goes over all source files in stdin and creates a GNU Make rule that 
# compiles them into an object files within env. variable $BIN_DIR
# @param stdin A list of source files
# @param $1 Compiler env. variable (e.g., CXX)
# @param $2 Flags env. variable (e.g., CXXFLAGS)
# @param $3 Suffix for generated mk file
# @param $4 Compiler
# @param $5 Flags
function shell_createmodule() {
    # Get compiler and flags
    compiler_text="\$($1)"
    flags_text="\$($2)"
    output=$BIN_DIR/objects_${3}.mk
    compiler="$4"
    flags="${@:5}"
	# Bypass if output exists
	[[ -e $output ]] && return
    # Create baseic Make rules
    files="$(cat -)"
    raw=
    for file in $files; do
		echo "Processing Make rule for $file..." >&2
#		     "(compiler: $compiler flags: $flags)..." >&2
        raw+=$(eval $compiler $flags -MM $file    |
               sed -E "s@^(.*):@${BIN_DIR}/\1:@g" |
               awk -v compiler="$compiler_text" -v flags="$flags_text" \
                   -v file="$file" '
                   NR>1 && /:/ {
                       printf "\t%s %s -o $@ -c $<\n", compiler, flags;
                       print $0
                   } NR==1 || !/:/ {
                       print $0
                   } END {
                       printf "\t%s %s -o $@ -c $<\n", compiler, flags
                   }')
        raw+="\n"
    done
    echo -e "$raw" > $output
#    echo -e "$raw" >&2
}
