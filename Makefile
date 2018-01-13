TARGET   := agent

BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects

CXX      := g++
CXXFLAGS := -std=c++14 -O2
CXXFLAGS += -DBACKTRACE
CXXFLAGS += -pedantic-errors -Wall
CXXFLAGS += -Wno-keyword-macro -Wno-pessimizing-move -Wno-unused-function

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

-include $(BUILD)/Makefile.dep

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(BUILD)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(OBJECTS) $(CXXFLAGS) $(INCLUDE) $(LDFLAGS) -o $(BUILD)/$(TARGET)

$(BUILD)/Makefile.dep: $(SRC)
	@echo "Depend"
	@mkdir -p $(@D)
	@for i in $(^); do $(CXX) $(CXXFLAGS) $(INCLUDE)  -MM "$${i}" -MT $(OBJ_DIR)/$${i%.*}.o; done > $@

.PHONY: all build clean depend

build:
	@mkdir -p $(OBJ_DIR)

clean:
	-rm -rf $(BUILD)


