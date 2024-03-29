# Introduction

*libcommon* contains data-structures and algorithms for C applications in Linux. The data-structures are thread-unsafe unless stated otherwise. See header files for additional information. 

# Prerequisits
1. A Linux operating system, or WSL for windows.
2. GNU gcc compiler compatible with C11.
3. GNU make, awk, sed, grep, cat utilities.

You can install all prerequisits using this command (in Ubuntu):
```bash
sudo apt-get install gcc awk sed make grep
```

# Using libcommon
Insert this repo as a submodule to your git repository
```bash
git submodule add https://github.com/alonrs/libcommon.git
```
Initialize the submodule using
```bash
git submodule update --init
```
Compile the library
```bash
make -C libcommon
```

# Contributing
Bug fixes, tests, and any more contributions are welcome.

# License
MIT license. See [LICENSE](LICENSE.MD) for more information.

