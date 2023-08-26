BUILD_DIR ?= ./out
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name *.cc)
OBJS_NATIVE := $(SRCS:%=$(BUILD_DIR)/native/%.o)
OBJS_WEB := $(SRCS:%=$(BUILD_DIR)/web/%.o)

DEPS := $(OBJS_NATIVE:.o=.d)
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
-include $(DEPS)

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -g -std=c++17

$(BUILD_DIR)/native/%.cc.o: %.cc
	mkdir -p $(dir $@)
	g++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/web/%.cc.o: %.cc
	mkdir -p $(dir $@)
	emcc $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

native: $(OBJS_NATIVE)
	g++ $(CPPFLAGS) $(OBJS_NATIVE) -o $(BUILD_DIR)/lox

# Requires emscripten installed
web: $(OBJS_WEB)
	emcc -sSTACK_SIZE=1048576 $(OBJS_WEB) -o $(BUILD_DIR)/lox.html

tests: native tests/tests.cc
	g++ $(CPPFLAGS) tests/tests.cc -o $(BUILD_DIR)/tests

all: native web tests

clean:
	$(RM) -r $(BUILD_DIR)

.PHONY: clean web native all tests
