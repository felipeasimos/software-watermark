
.PHONY: test lib debug build run coverage avg loc compile_flags docs init clean clean-objs help
help:
	# This Makefile can produce a dynamic library, test binary and main executable for C/C++ projects.

	# The only prerequisite is that you must have this project structure:
	# myproject/
	# ├── Makefile
	# ├── main.c (optional)
	# ├── include/
	# ├── src/ (can contain one level of subfolders)
	# └── tests/ (can contain one level of subfolders)

	# the available commands in this Makefile are:
	# test - build and run tests with debug flags and debugger
	# lib - build dynamic lib for release
	# debug - build and run main executable (if you have one) with debug flags and debugger
	# build - build main executable for release (if you have one)
	# run - build and run main executable for release (if you have one)
	# coverage - run code coverage command
	# avg - shows average code coverage from percentages displayed by coverage command to stdout
	# loc - show sum of lines in the src/ and include/ directories
	# compile_flags - generate compile_flags.txt file for clangd lsp
	# docs - generate documentation
	# init - create minimal project structure in current folder (src/, tests/, include/)
	# clean - remove build/ (folder where targets are stored)
	# clean-objs - remove build/ subfolder with objects generate from src/
	# help - show this help

# compilation variables to be set per project
# v v v v v v v v v v v v v v v v v v v v v v

# compiler being used
# COMPILER :=clang -fopenmp
COMPILER :=clang

# flags that will be used in every compilation command
FLAGS :=-std=c99

# command used to generate documentation (you can leave it empty)
DOCUMENTATION_COMMAND :=

# target name. Will be used to name the library, executable
# and test binaries:
# build/lib/libtarget.so, build/main/target, build/tests/target_test
TARGET :=watermark

# debugger command to run when debugging (you can leave it empty)
DEBUGGER_COMMAND :=valgrind --leak-check=full --exit-on-first-error=yes --error-exitcode=1 --quiet

# code coverage command to use (you can leave it empty)
CODE_COVERAGE_COMMAND :=llvm-cov-11 gcov -n

# these can be either c or cpp
TESTS_FILE_EXTENSION :=c
SRC_FILE_EXTENSION :=c

# libraries to link against (without -l)
GENERAL_LIBS :=
MAIN_LIBS :=
TESTS_LIBS :=

# libs location (if they are system libs, just leave it empty)
GENERAL_LIBS_LOCATION :=
MAIN_LIBS_LOCATION :=
TESTS_LIBS_LOCATION :=

# additional headers location
GENERAL_HEADERS_LOCATION :=
MAIN_HEADERS_LOCATION :=
TESTS_HEADERS_LOCATION :=./tests

# general flags
# FLAGS := -fPIC
FLAGS := -fPIC -pedantic-errors -Wall -Wextra  -Werror
DEBUG_FLAGS :=-DDEBUG -O0 -g
RELEASE_FLAGS :=-O3 -DNDEBUG
TESTS_FLAGS :=--coverage

# ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^
# compilation variables to be set per project

# define directories
ROOT_DIR :=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
SRC_DIR :=$(ROOT_DIR)/src
INCLUDE_DIR :=$(ROOT_DIR)/include
BUILD_DIR :=$(ROOT_DIR)/build
OBJ_DIR :=$(BUILD_DIR)/objs
MAIN_DIR :=$(BUILD_DIR)/main
LIB_DIR :=$(BUILD_DIR)/lib
TESTS_MAIN_DIR :=$(BUILD_DIR)/tests
TESTS_SRC_DIR :=$(ROOT_DIR)/tests
TESTS_OBJ_DIR :=$(TESTS_MAIN_DIR)/objs

# define sources
SRC :=$(wildcard $(SRC_DIR)/*.$(SRC_FILE_EXTENSION)) $(wildcard $(SRC_DIR)/*/*.$(SRC_FILE_EXTENSION))
SRC :=$(wildcard $(SRC_DIR)/*.$(SRC_FILE_EXTENSION)) $(wildcard $(SRC_DIR)/*/*.$(SRC_FILE_EXTENSION))
TESTS_SRC :=$(wildcard $(TESTS_SRC_DIR)/*.$(TESTS_FILE_EXTENSION)) $(wildcard $(TESTS_SRC_DIR)/*/*.$(TESTS_FILE_EXTENSION))
MAIN_SRC :=$(ROOT_DIR)/$(TARGET).$(SRC_FILE_EXTENSION)

# define targets
TARGET_LIB :=$(LIB_DIR)/lib$(TARGET).so
TARGET_MAIN :=$(MAIN_DIR)/$(TARGET)
TARGET_TESTS :=$(TESTS_MAIN_DIR)/$(TARGET)_tests

# define intermediary objects
SRC_OBJS :=$(SRC:$(SRC_DIR)/%.$(SRC_FILE_EXTENSION)=$(OBJ_DIR)/%.o)
TESTS_OBJS := $(TESTS_SRC:$(TESTS_SRC_DIR)/%.$(TESTS_FILE_EXTENSION)=$(TESTS_OBJ_DIR)/%.o)

# add -l to library names
GENERAL_LIBS :=$(addprefix -l,$(GENERAL_LIBS))
MAIN_LIBS :=$(addprefix -l,$(MAIN_LIBS)) $(GENERAL_LIBS)
TESTS_LIBS :=$(addprefix -l,$(TESTS_LIBS)) $(GENERAL_LIBS)

# add -L to library directories
GENERAL_LIBS_LOCATION :=$(addprefix -L,$(GENERAL_LIBS_LOCATION))
MAIN_LIBS_LOCATION :=$(addprefix -L,$(MAIN_LIBS_LOCATION)) $(GENERAL_LIBS_LOCATION)
TESTS_LIBS_LOCATION :=$(addprefix -L,$(TESTS_LIBS)) $(GENERAL_LIBS_LOCATION)

# add -I to header directories
GENERAL_HEADERS_LOCATION_WITH_FLAG :=$(addprefix -I,$(GENERAL_HEADERS_LOCATION)) $(addprefix -I,$(INCLUDE_DIR))
MAIN_HEADERS_LOCATION_WITH_FLAG :=$(addprefix -I,$(MAIN_HEADERS_LOCATION)) $(GENERAL_HEADERS_LOCATION_WITH_FLAG)
TESTS_HEADERS_LOCATION_WITH_FLAG :=$(addprefix -I,$(TESTS_HEADERS_LOCATION)) $(GENERAL_HEADERS_LOCATION_WITH_FLAG)

# build objects
$(SRC_OBJS): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(COMPILER) $(FLAGS) $(GENERAL_HEADERS_LOCATION_WITH_FLAG) $< -c -o $@ $(GENERAL_LIBS_LOCATION) $(GENERAL_LIBS)

# build tests objects
$(TESTS_OBJS): $(SRC_OBJS)
$(TESTS_OBJS): $(TESTS_OBJ_DIR)/%.o: $(TESTS_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(COMPILER) $(FLAGS) $(TESTS_HEADERS_LOCATION_WITH_FLAG) $< -c -o $@ $(TESTS_LIBS_LOCATION) $(TESTS_LIBS)

# build lib
$(TARGET_LIB): $(SRC_OBJS)
	@mkdir -p $(dir $@)
	$(COMPILER) $(FLAGS) $(GENERAL_HEADERS_LOCATION_WITH_FLAG) $^ -shared -o $@
	$(MAKE) -C $(ROOT_DIR) clean-objs

# build tests executable
$(TARGET_TESTS): $(TESTS_OBJS)
	@mkdir -p $(dir $@)
	$(COMPILER) $(FLAGS) $(TESTS_HEADERS_LOCATION_WITH_FLAG) $(SRC_OBJS) $^ -o $@

# build main
$(TARGET_MAIN): $(SRC_OBJS) $(MAIN_SRC)
	@mkdir -p $(dir $@)
	$(COMPILER) $(FLAGS) $(MAIN_HEADERS_LOCATION_WITH_FLAG) $^ -o $@ $(MAIN_LIBS_LOCATION) $(MAIN_LIBS)
	$(MAKE) -C $(ROOT_DIR) clean-objs

# main commands
lib: clean
lib: FLAGS+=$(RELEASE_FLAGS)
lib: $(TARGET_LIB)

test: FLAGS+=$(DEBUG_FLAGS) $(TESTS_FLAGS)
test: $(TARGET_TESTS)
	$(DEBUGGER_COMMAND) $(TARGET_TESTS)

debug: FLAGS+=$(DEBUG_FLAGS)
debug: $(SRC_OBJS) $(TARGET_MAIN)
	$(DEBUGGER_COMMAND) $(TARGET_MAIN)

build: clean
build: FLAGS+=$(RELEASE_FLAGS)
build: $(SRC_OBJS) $(TARGET_MAIN)

run: build
	$(TARGET_MAIN)

coverage:
	@$(CODE_COVERAGE_COMMAND) $(SRC_OBJS)

avg:
	@$(MAKE) coverage | grep -Eo "[[:digit:]]{0,3}\.[[:digit:]]{0,2}%" | \
		sed 's/%//' | \
		awk -v CONVFMT=%.4g '{ s+=$$1 } END { print s/NR "%" }'
loc:
	@find $(SRC_DIR) $(INCLUDE_DIR) -exec cat {} + 2>/dev/null | grep -Ev "(^$$)|(^//)|(^/\*)" | wc -l

compile_flags:
	@echo "-I" > $(ROOT_DIR)/compile_flags.txt
	@echo "$(INCLUDE_DIR)" >> $(ROOT_DIR)/compile_flags.txt
	@for folder in $(TESTS_HEADERS_LOCATION) $(MAIN_HEADERS_LOCATION); do\
		echo "-I" >> $(ROOT_DIR)/compile_flags.txt;\
		echo $$folder >> $(ROOT_DIR)/compile_flags.txt;\
	done
	@echo "-DDEBUG" >> $(ROOT_DIR)/compile_flags.txt
	@for opt in $(FLAGS); do\
		echo $$opt >> $(ROOT_DIR)/compile_flags.txt;\
	done

docs: 
	$(DOCUMENTATION_COMMAND)

init:
	@mkdir -p $(INCLUDE_DIR)
	@mkdir -p $(SRC_DIR)
	@mkdir -p $(TESTS_SRC_DIR)
clean:
	@-rm -rf $(BUILD_DIR)

# clean build/objs/ to avoid debugging objects with release flags later or build for release with debug flags
clean-objs:
	@-rm -rf $(OBJ_DIR)

