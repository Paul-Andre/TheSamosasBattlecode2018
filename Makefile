TARGET   := agent

BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects

CXX      := -g++
CXXFLAGS := -std=c++14 -pedantic-errors -Wall -Wno-keyword-macro -O3
LDFLAGS  := -L../battlecode/c/lib -lm -lc
INCLUDE  := -I../battlecode/c/include -Iinclude
SRC      := \
	$(wildcard src/*.cpp) \
	$(wildcard lib/*.cpp)

OBJECTS := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
	LDFLAGS += -lbattlecode-linux -lutil -ldl -lrt -lgcc_s -pthread
endif
ifeq ($(UNAME_S), Darwin)
	LDFLAGS += -lbattlecode-darwin -lSystem -lresolv
endif

all: build $(BUILD)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(BUILD)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(OBJECTS) $(CXXFLAGS) $(INCLUDE) $(LDFLAGS) -o $(BUILD)/$(TARGET)

.PHONY: all build clean

build:
	@mkdir -p $(OBJ_DIR)

clean:
	-rm -rf $(BUILD)
