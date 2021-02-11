ROOT_DIR :=.
SRC_DIR := $(ROOT_DIR)/src
INCLUDE_DIR := $(ROOT_DIR)/include
BUILD_DIR := $(ROOT_DIR)/build
OBJ_DIR := $(BUILD_DIR)/objects
APP_DIR := $(BUILD_DIR)/app
TEST_OBJ_DIR := $(BUILD_DIR)/tests
TEST_SRC_DIR := $(ROOT_DIR)/tests

TEST_FILE_EXTENSION := c
SRC_FILE_EXTENSION := c

# targets and pre-requisites
SRC := $(wildcard $(SRC_DIR)/*/*.$(SRC_FILE_EXTENSION))
OBJECTS := $(SRC:$(SRC_DIR)/%.$(SRC_FILE_EXTENSION)=$(OBJ_DIR)/%.o)

TESTS_SRC := $(wildcard $(TEST_SRC_DIR)/*/*.$(TEST_FILE_EXTENSION))
TESTS_OBJ := $(TESTS_SRC:$(TEST_SRC_DIR)/%.$(TEST_FILE_EXTENSION)=$(TEST_OBJ_DIR)/%.o)

TARGET := libwatermark.so
TEST_TARGET := test

# libraries (without -l)
LIBS :=
TEST_LIBS :=

# compiler settings
CPP := c99
CPPFLAG := -pedantic-errors -Wall -Wextra -Werror -fPIC
LDFLAGS := $(addprefix -l,$(LIBS))
INCLUDE := -I$(INCLUDE_DIR)

TEST_LIBS := $(addprefix -l,$(TEST_LIBS))

# final preparations (don't change this)
TARGET_FINAL := $(APP_DIR)/$(TARGET)
TEST_TARGET_FINAL := $(ROOT_DIR)/$(TEST_TARGET)

# tests, static analysis and code coverage
GCOV_COMMAND:=gcov -rnm $(TESTS_OBJ) $(OBJECTS) 2>/dev/null | grep -Eo "('.*'|[[:digit:]]{1,3}.[[:digit:]]{2}%)" | paste -d " " - - | sort -k2 -nr
UNTESTED_DETECTOR_COMMAND:=$(GCOV_COMMAND) | grep -v "100.00%" | awk '{ print "\x1b[38;2;255;25;25;1m" $$1 " \x1b[0m\x1b[38;2;255;100;100;1m" $$2 "\x1b[0m" }'
COVERAGE_COMMAND:=@$(UNTESTED_DETECTOR_COMMAND); $(GCOV_COMMAND) | awk '{ sum += $$2; count[NR] = $$2 } END { if(NR%2) { median=count[(NR+1)/2]; } else { median=count[NR/2]; } if( NR==0 ) { NR=1; } print "\x1b[32mCode Coverage\x1b[0m:\n\t\x1b[33mAverage\x1b[0m: " sum/NR "%\n\x1b[35m\tMedian\x1b[0m: " median  }'
RUN_TESTS_COMMAND:=@valgrind -q --exit-on-first-error=yes --error-exitcode=1 --tool=memcheck\
		--show-reachable=yes --leak-check=yes --track-origins=yes $(TEST_TARGET_FINAL) --gtest_color=1 | grep -Pv "^\x1b\[0;32m" | grep -v "^$$"
STATIC_ANALYSIS_COMMAND:=@cppcheck --addon=cert --addon=threadsafety --addon=naming \
	$(INCLUDE) --suppress=missingIncludeSystem --quiet --enable=all $(SRC) $(TESTS_SRC)

SHELL := /bin/bash
.PHONY: all folders clean debug release test spy profile hist

release: CPPFLAGS += -O2 -fPIC
release: | clean all 

all: folders $(TARGET_FINAL)

debug: CPPFLAGS += -DDEBUG -g -fPIC
debug: COVERAGE = --coverage
debug: $(TESTS_OBJ) test all

test: LDFLAGS += $(TEST_LIBS)
test: $(TESTS_OBJ) $(OBJECTS) 
	@$(CPP) $(CPPFLAGS) $(INCLUDE) $(COVERAGE) -o $(TEST_TARGET_FINAL) $^ $(LDFLAGS)
	$(STATIC_ANALYSIS_COMMAND)
	$(RUN_TESTS_COMMAND)
	$(COVERAGE_COMMAND)

folders:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(TEST_OBJ_DIR)

clean:
	-@rm -vf spy
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*
	-@rm -rvf $(TEST_OBJ_DIR)/*
	-@rm -vf $(TEST_TARGET_FINAL)

$(TARGET_FINAL): $(OBJECTS)
	@mkdir -p $(@D)
	@$(CPP) $(CPPFLAGS) -shared -o $(TARGET_FINAL) $^ $(LDFLAGS)

$(OBJECTS): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.$(SRC_FILE_EXTENSION)
	@mkdir -p $(@D)
	@$(CPP) $(CPPFLAGS) $(INCLUDE) $(COVERAGE) -c $< -o $@

$(TESTS_OBJ): $(OBJECTS)
$(TESTS_OBJ): $(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.$(TEST_FILE_EXTENSION)
	@mkdir -p $(@D)
	@$(CPP) $(CPPFLAGS) $(INCLUDE) -c $< -o $@
