# Flags
CXXFLAGS := -Wall -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-private-field -Wno-register -Wno-c++11-narrowing -Wno-delete-non-abstract-non-virtual-dtor -Wno-format -std=c++20
FFLAGS :=
BFLAGS := -d -v
LOGMODE :=


DEBUG_CXXFLAGS := $(CXXFLAGS) -g -O0
# LOGMODE = DEBUGMODE

CXXFLAGS += -O3 -pg
# LOGMODE = DEBUGMODE


# CXXFLAGS += -D DEBUGMODE 
DEBUG_CXXFLAGS += -D DEBUGMODE 
# Compilers
CXX := clang++
FLEX := flex
BISON := bison

# Directories
TOP_DIR := $(shell pwd)
TARGET_EXEC := exp
DEBUG_EXEC := debug.out
SRC_DIR := $(TOP_DIR)/src
EXEC_DIR ?= $(TOP_DIR)/exec
BUILD_DIR ?= $(TOP_DIR)/build
DEBUG_DIR ?= $(TOP_DIR)/debug

# Source files & target files
BINARY := exp
CPPFILES  := $(shell find $(SRC_DIR) -name "*.cpp")
LEXFILES  := $(shell find $(SRC_DIR) -name "*.l")
YACCFILES := $(shell find $(SRC_DIR) -name "*.y")
FB_GEN := $(patsubst $(SRC_DIR)/%.l, $(BUILD_DIR)/%.lex.cpp, $(LEXFILES))
FB_GEN += $(patsubst $(SRC_DIR)/%.y, $(BUILD_DIR)/%.tab.cpp, $(YACCFILES))
SRC := $(FB_GEN) $(CPPFILES)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.cpp.o, $(SRC))
OBJECTS := $(patsubst $(BUILD_DIR)/%.cpp, $(BUILD_DIR)/%.cpp.o, $(OBJECTS))

DEBUG_FB_GEN := $(patsubst $(SRC_DIR)/%.l, $(DEBUG_DIR)/%.lex.cpp, $(LEXFILES))
DEBUG_FB_GEN += $(patsubst $(SRC_DIR)/%.y, $(DEBUG_DIR)/%.tab.cpp, $(YACCFILES))
DEBUG_OBJECTS := $(patsubst $(BUILD_DIR)/%.cpp.o, $(DEBUG_DIR)/%.cpp.o, $(OBJECTS))

# Header directories & dependencies
INC_DIRS := $(shell find $(SRC_DIR) -type d)
INC_DIRS += $(INC_DIRS:$(SRC_DIR)%=$(BUILD_DIR)%)
INC_FLAGS := $(addprefix -I, $(INC_DIRS))
DEPS := $(OBJS:.o=.d)
CPPFLAGS = $(INC_FLAGS) -MMD -MP

DEBUG_INC_DIRS := $(shell find $(SRC_DIR) -type d)
DEBUG_INC_DIRS += $(DEBUG_INC_DIRS:$(SRC_DIR)%=$(DEBUG_DIR)%)
DEBUG_INC_FLAGS := $(addprefix -I, $(DEBUG_INC_DIRS))
DEBUG_CPPFLAGS = $(DEBUG_INC_FLAGS) -MMD -MP


all: $(FB_GEN) $(OBJECTS)
	mkdir -p $(EXEC_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread -ldl -o $(EXEC_DIR)/$(TARGET_EXEC) $(OBJECTS)

debug: $(DEBUG_FB_GEN) $(DEBUG_OBJECTS)
	mkdir -p $(EXEC_DIR)
	$(CXX) $(DEBUG_CPPFLAGS) $(DEBUG_CXXFLAGS) -lpthread -ldl -o $(EXEC_DIR)/$(DEBUG_EXEC) $(DEBUG_OBJECTS)

# flex
$(BUILD_DIR)/%.lex.cpp: $(SRC_DIR)/%.l
	mkdir -p $(dir $@)
	$(FLEX) $(FFLAGS) -o $@ $<

# bison
$(BUILD_DIR)/%.tab.cpp: $(SRC_DIR)/%.y
	mkdir -p $(dir $@)
	$(BISON) $(BFLAGS) -o $@ $<

# flex
$(DEBUG_DIR)/%.lex.cpp: $(SRC_DIR)/%.l
	mkdir -p $(dir $@)
	$(FLEX) $(FFLAGS) -o $@ $<

# bison
$(DEBUG_DIR)/%.tab.cpp: $(SRC_DIR)/%.y
	mkdir -p $(dir $@)
	$(BISON) $(BFLAGS) -o $@ $<

# cpp object
# .PHONY: $(BUILD_DIR)/main.cpp.o
# $(BUILD_DIR)/main.cpp.o: $(SRC_DIR)/main.cpp
# 	mkdir -p $(dir $@)
# 	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.cpp.o: $(BUILD_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# .PHONY: $(DEBUG_DIR)/main.cpp.o
# $(DEBUG_DIR)/main.cpp.o: $(SRC_DIR)/main.cpp
# 	mkdir -p $(dir $@)
# 	$(CXX) $(DEBUG_CPPFLAGS) $(DEBUG_CXXFLAGS) -c $< -o $@

$(DEBUG_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(DEBUG_CPPFLAGS) $(DEBUG_CXXFLAGS) -c $< -o $@

$(DEBUG_DIR)/%.cpp.o: $(DEBUG_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(DEBUG_CPPFLAGS) $(DEBUG_CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	-rm -rf $(BUILD_DIR) $(DEBUG_DIR)

.PHONY: test

test:
	$(EXEC_DIR)/$(TARGET_EXEC)

.PHONY: run

run:
	mkdir -p output
	nohup $(EXEC_DIR)/$(TARGET_EXEC) > output/$(TARGET_EXEC).txt &

.PHONY: debug_run

debug_run:
	gdb $(EXEC_DIR)/$(DEBUG_EXEC)

