# v Settings to be changed per-project v
# v v v v v v v v v vvvv v v v v v v v v

ROOT_DIR :=.
TEST_FILE_EXTENSION := c
SRC_FILE_EXTENSION := c

TARGET := watermark
TEST_TARGET := test

# libraries (without -l)
LIBS :=
MAIN_LIBS :=
TEST_LIBS :=

# libraries locations
LIBS_LOCATION :=
MAIN_LIBS_LOCATION :=
TEST_LIBS_LOCATION :=

# ^ Settings to be changed per-project ^
# ^ ^ ^ ^ ^ ^ ^ ^ ^ ^^^^ ^ ^ ^ ^ ^ ^ ^ ^ 

ROOT_DIR :=.
SRC_DIR := $(ROOT_DIR)/src
INCLUDE_DIR := $(ROOT_DIR)/include
BUILD_DIR := $(ROOT_DIR)/build
OBJ_DIR := $(BUILD_DIR)/objects
APP_DIR := $(BUILD_DIR)/app
TEST_OBJ_DIR := $(BUILD_DIR)/tests
TEST_SRC_DIR := $(ROOT_DIR)/tests
THIRD_PARTY_OBJS_DIR := $(ROOT_DIR)/objs

# targets and pre-requisites
SRC := $(wildcard $(SRC_DIR)/*/*.$(SRC_FILE_EXTENSION))
OBJECTS := $(SRC:$(SRC_DIR)/%.$(SRC_FILE_EXTENSION)=$(OBJ_DIR)/%.o) $(wildcard $(THIRD_PARTY_OBJS_DIR)/*.o)

TESTS_SRC := $(wildcard $(TEST_SRC_DIR)/*/*.$(TEST_FILE_EXTENSION))
TESTS_OBJ := $(TESTS_SRC:$(TEST_SRC_DIR)/%.$(TEST_FILE_EXTENSION)=$(TEST_OBJ_DIR)/%.o)

TARGET_LIB := lib$(TARGET).so

MAIN := $(TARGET).$(SRC_FILE_EXTENSION)

# libraries (without -l)
MAIN_LIBS := $(TARGET) $(MAIN_LIBS)

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
CC := c99
CFLAGS := -pedantic-errors -Wall -Wextra -Werror -fPIC
LDFLAGS :=
INCLUDE := -I$(INCLUDE_DIR)

LIBS := $(LIBS_LOCATION) $(addprefix -l,$(LIBS))
MAIN_LIBS := $(MAIN_LIBS_LOCATION) $(addprefix -l,$(MAIN_LIBS))
TEST_LIBS := $(TEST_LIBS_LOCATION) $(addprefix -l,$(TEST_LIBS))

# final preparations (don't change this)
TARGET_LIB_FINAL := $(APP_DIR)/$(TARGET_LIB)
TEST_TARGET_FINAL := $(ROOT_DIR)/$(TEST_TARGET)

# tests, static analysis and code coverage
GCOV_COMMAND:=gcov -rnm $(TESTS_OBJ) $(OBJECTS) 2>/dev/null | grep -Eo "('.*'|[[:digit:]]{1,3}.[[:digit:]]{2}%)" | paste -d " " - - | sort -k2 -nr
UNTESTED_DETECTOR_COMMAND:=$(GCOV_COMMAND) | grep -v "100.00%" | awk '{ print "\x1b[38;2;255;25;25;1m" $$1 " \x1b[0m\x1b[38;2;255;100;100;1m" $$2 "\x1b[0m" }'
COVERAGE_COMMAND:=@$(UNTESTED_DETECTOR_COMMAND); $(GCOV_COMMAND) | awk '{ sum += $$2; count[NR] = $$2 } END { if(NR%2) { median=count[(NR+1)/2]; } else { median=count[NR/2]; } if( NR==0 ) { NR=1; } print "\x1b[32mCode Coverage\x1b[0m:\n\t\x1b[33mAverage\x1b[0m: " sum/NR "%\n\x1b[35m\tMedian\x1b[0m: " median  }'
RUN_TESTS_COMMAND:=@valgrind -q --exit-on-first-error=yes --error-exitcode=1 --tool=memcheck\
		--show-reachable=yes --leak-check=yes --track-origins=yes $(TEST_TARGET_FINAL) --gtest_color=1 | grep -Pv "^\x1b\[0;32m" | grep -v "^$$"
STATIC_ANALYSIS_COMMAND:=@cppcheck --addon=cert --addon=threadsafety --addon=naming \
	$(INCLUDE) --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=knownConditionTrueFalse --quiet --enable=all $(SRC) $(TESTS_SRC)

SHELL := /bin/bash
.PHONY: lib folders clean debug release test profile hist main compilemain

# build lib, run tests, compile and run main
all: | debug lib main

release: CFLAGS += -O2 -fPIC
release: | clean lib 

compilemain: LDFLAGS += $(MAIN_LIBS)
compilemain: release
	@$(CC) $(CFLAGS) $(INCLUDE) $(MAIN) -o $(TARGET) $(LDFLAGS)

main: release compilemain
	LD_LIBRARY_PATH=$(MAIN_LIBS_DIR_LOCATION) ./$(TARGET)

lib: folders $(TARGET_LIB_FINAL)

debug: CFLAGS += -DDEBUG -g -fPIC
debug: COVERAGE = --coverage
debug: $(TESTS_OBJ) test lib

test: $(TESTS_OBJ) $(OBJECTS) 
	@$(CC) $(CFLAGS) $(INCLUDE) $(COVERAGE) -o $(TEST_TARGET_FINAL) $^ $(LDFLAGS)
	$(STATIC_ANALYSIS_COMMAND)
	$(RUN_TESTS_COMMAND)
	$(COVERAGE_COMMAND)

folders:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(TEST_OBJ_DIR)

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*
	-@rm -rvf $(TEST_OBJ_DIR)/*
	-@rm -vf $(TEST_TARGET_FINAL)

$(TARGET_LIB_FINAL): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -shared -o $(TARGET_LIB_FINAL) $^ $(LDFLAGS)

$(OBJECTS): LDFLAGS += $(LIBS)
$(OBJECTS): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.$(SRC_FILE_EXTENSION)
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(INCLUDE) $(COVERAGE) -c $< -o $@

$(TESTS_OBJ): $(OBJECTS)
$(TESTS_OBJ): LDFLAGS += $(TEST_LIBS)
$(TESTS_OBJ): $(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.$(TEST_FILE_EXTENSION)
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
