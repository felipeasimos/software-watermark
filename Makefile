# v Settings to be changed per-project v
# v v v v v v v v v vvvv v v v v v v v v

# compiler settings
CC := clang
CFLAGS = -pedantic-errors -Wall -Wextra -std=c99 -fPIC -Werror

# extensions and location
ROOT_DIR :=.
TEST_FILE_EXTENSION := c
SRC_FILE_EXTENSION := c

# final binaries
## project name, will also be the name of the main binary and library
TARGET := watermark
## name of the test binary
TEST_TARGET := test

# libraries (without -l)
LIBS :=
MAIN_LIBS :=
TEST_LIBS :=

# gcov-like coverage command (expect gcov output, not gcov files)
# Examples: llvm-cov-7 gcov -n (clang), gcov -rnm (gcc)
# files with 100% coverage are not showed in the final output
COVERAGE_COMMAND := llvm-cov-7 gcov -n

# libraries locations
LIBS_LOCATION :=
MAIN_LIBS_LOCATION :=
TEST_LIBS_LOCATION :=

# ^ Settings to be changed per-project ^
# ^ ^ ^ ^ ^ ^ ^ ^ ^ ^^^^ ^ ^ ^ ^ ^ ^ ^ ^ 

SRC_DIR := $(ROOT_DIR)/src
INCLUDE_DIR := $(ROOT_DIR)/include
BUILD_DIR := $(ROOT_DIR)/build
OBJ_DIR := $(BUILD_DIR)/objects
APP_DIR := $(BUILD_DIR)/app
TEST_OBJ_DIR := $(BUILD_DIR)/tests
TEST_SRC_DIR := $(ROOT_DIR)/tests

# targets and pre-requisites
SRC := $(wildcard $(SRC_DIR)/*/*.$(SRC_FILE_EXTENSION))
OBJECTS := $(SRC:$(SRC_DIR)/%.$(SRC_FILE_EXTENSION)=$(OBJ_DIR)/%.o)

TESTS_SRC := $(wildcard $(TEST_SRC_DIR)/*/*.$(TEST_FILE_EXTENSION))
TESTS_OBJ := $(TESTS_SRC:$(TEST_SRC_DIR)/%.$(TEST_FILE_EXTENSION)=$(TEST_OBJ_DIR)/%.o)

TARGET_LIB := lib$(TARGET).so

MAIN := $(ROOT_DIR)/$(TARGET).$(SRC_FILE_EXTENSION)

# libraries (without -l)
MAIN_LIBS := $(TARGET) $(MAIN_LIBS)

# target with address
TARGET := $(ROOT_DIR)/$(TARGET)

# libraries location
LIBS_LOCATION :=
MAIN_LIBS_LOCATION :=$(APP_DIR) $(MAIN_LIB_LOCATION) 

# libraries directories
LIBS_DIR_LOCATION := $(LIBS_LOCATION)
MAIN_LIBS_DIR_LOCATION := $(MAIN_LIBS_LOCATION)

# libraries settings
LIBS_LOCATION := $(addprefix -L,$(LIBS_LOCATION))
MAIN_LIBS_LOCATION := $(addprefix -L,$(MAIN_LIBS_LOCATION))
TEST_LIBS_LOCATION := $(addprefix -L,$(TEST_LIBS_LOCATION))

# compiler settings
LDFLAGS =
INCLUDE := -I$(INCLUDE_DIR)

LIBS := $(LIBS_LOCATION) $(addprefix -l,$(LIBS))
MAIN_LIBS := $(MAIN_LIBS_LOCATION) $(addprefix -l,$(MAIN_LIBS))
TEST_LIBS := $(TEST_LIBS_LOCATION) $(addprefix -l,$(TEST_LIBS))

# final preparations (don't change this)
TARGET_LIB_FINAL := $(APP_DIR)/$(TARGET_LIB)
TEST_TARGET_FINAL := $(ROOT_DIR)/$(TEST_TARGET)

# tests, static analysis and code coverage
GCOV_COMMAND:=$(COVERAGE_COMMAND) $(TESTS_OBJ) $(OBJECTS) 2>/dev/null | grep -Eo "('.*'|[[:digit:]]{1,3}.[[:digit:]]{2}%)" | paste -d " " - - | sort -k2 -nr
UNTESTED_DETECTOR_COMMAND:=$(GCOV_COMMAND) | grep -v "100.00%" | grep -v "^'tests/" | awk '{ print "\x1b[38;2;255;25;25;1m" $$1 " \x1b[0m\x1b[38;2;255;100;100;1m" $$2 "\x1b[0m" }'
COVERAGE_COMMAND:=@$(UNTESTED_DETECTOR_COMMAND); $(GCOV_COMMAND) | awk '{ sum += $$2; count[NR] = $$2 } END { if(NR%2) { median=count[(NR+1)/2]; } else { median=count[NR/2]; } if( NR==0 ) { NR=1; } print "\x1b[32mCode Coverage\x1b[0m:\n\t\x1b[33mAverage\x1b[0m: " sum/NR "%\n\x1b[35m\tMedian\x1b[0m: " median  }'
RUN_TESTS_COMMAND:=@valgrind -q --exit-on-first-error=yes --error-exitcode=1 --tool=memcheck\
		--show-reachable=yes --leak-check=yes --track-origins=yes $(TEST_TARGET_FINAL) #--gtest_color=1 | grep -Pv "^\x1b\[0;32m" | grep -v "^$$"
STATIC_ANALYSIS_COMMAND:=@cppcheck --addon=cert --addon=threadsafety --addon=naming \
	$(INCLUDE) --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=knownConditionTrueFalse --quiet --enable=all $(SRC) $(TESTS_SRC)

SHELL := /bin/bash
.PHONY: help lib folders clean debug release test profile main compilemain

# build lib, run tests, compile and run main
all: | debug lib main

help:
	# release - produce optimized lib
	# compilemain - produce main
	# main - produce main and run it
	# debug - produce lib with debug info and run tests
	# clean - clean produced binaries
	# clangd - generate compile_flags.txt file for clangd
plot:
	gnuplot -e "plot '$(TEST_SRC_DIR)/report_rm_1.plt' using 1:2 with lines,\
		'$(TEST_SRC_DIR)/report_rm_2.plt' using 1:2 with lines,\
		'$(TEST_SRC_DIR)/report_rm_3.plt' using 1:2 with lines,\
		'$(TEST_SRC_DIR)/report_rm_4.plt' using 1:2 with lines,\
		'$(TEST_SRC_DIR)/report_rm_5.plt' using 1:2 with lines,\
		'$(TEST_SRC_DIR)/report_rm_6.plt' using 1:2 with lines,\
		'$(TEST_SRC_DIR)/report_rm_7.plt' using 1:2 with lines; pause -1 \"Hit any key to continue\""

clangd:
	@echo "-I" > $(ROOT_DIR)/compile_flags.txt
	@echo "$(INCLUDE_DIR)" >> $(ROOT_DIR)/compile_flags.txt
	@for opt in $(CFLAGS); do\
		echo $$opt >> $(ROOT_DIR)/compile_flags.txt;\
	done

release: CFLAGS += -O2 -fPIC
release: | clean lib 

compilemain: LDFLAGS = $(MAIN_LIBS)
compilemain: release
	@$(CC) $(CFLAGS) $(INCLUDE) $(MAIN) -o $(TARGET) $(LDFLAGS)

main: release compilemain
	LD_LIBRARY_PATH=$(MAIN_LIBS_DIR_LOCATION) $(TARGET)

lib: folders $(TARGET_LIB_FINAL)

debug: CFLAGS += -DDEBUG -g -O0 -fprofile-arcs -ftest-coverage
debug: LDFLAGS = $(LIBS) $(TEST_LIBS) --coverage
debug: $(TESTS_OBJ) test lib

test: $(TESTS_OBJ) $(OBJECTS) 
	@$(CC) $(CFLAGS) $(INCLUDE) -o $(TEST_TARGET_FINAL) $^ $(LDFLAGS)
	@#$(STATIC_ANALYSIS_COMMAND)
	$(RUN_TESTS_COMMAND)
	$(COVERAGE_COMMAND)

folders:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(TEST_OBJ_DIR)

clean:
	# removed build/ directory, $(TEST_TARGET_FINAL) and $(TARGET)
	-@rm -rf $(BUILD_DIR)/*
	-@rm -f $(TEST_TARGET_FINAL)
	-@rm -f $(TARGET)

$(TARGET_LIB_FINAL): $(OBJECTS)
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -shared -o $(TARGET_LIB_FINAL) $^ $(LDFLAGS)

$(OBJECTS): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.$(SRC_FILE_EXTENSION)
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(INCLUDE) $(COVERAGE) -c $< -o $@

$(TESTS_OBJ): $(OBJECTS)
$(TESTS_OBJ): $(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.$(TEST_FILE_EXTENSION)
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
