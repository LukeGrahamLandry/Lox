BUILD_DIR ?= ./out
SRC_DIRS ?= ./src

SRCS_NATIVE := $(shell find $(SRC_DIRS) -name *.cc -and -not -name web.cc)
SRCS_WEB := $(shell find $(SRC_DIRS) -name *.cc -and -not -name main.cc)
OBJS_DEBUG := $(SRCS_NATIVE:%=$(BUILD_DIR)/debug/%.o)
OBJS_NATIVE := $(SRCS_NATIVE:%=$(BUILD_DIR)/native/%.o)
OBJS_WEB := $(SRCS_WEB:%=$(BUILD_DIR)/web/%.o)

# this makes it cope with changes header files apparently?
DEPS := $(OBJS_NATIVE:.o=.d)
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
-include $(DEPS)

CXXFLAGS := $(INC_FLAGS) -MMD -MP -std=c++17
RELEASE_FLAGS := -O3
DEBUG_FLAGS := -g -fsanitize=address -fno-omit-frame-pointer -Wno-format-security -fsanitize=undefined -DDEBUG_STRESS_GC
WEB_FLAGS := -Oz

$(BUILD_DIR)/native/%.cc.o: %.cc
	mkdir -p $(dir $@)
	g++ $(RELEASE_FLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/debug/%.cc.o: %.cc
	mkdir -p $(dir $@)
	g++ $(DEBUG_FLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/web/%.cc.o: %.cc
	mkdir -p $(dir $@)
	emcc $(WEB_FLAGS) $(CXXFLAGS) -c $< -o $@

native: $(OBJS_NATIVE)
	g++ $(RELEASE_FLAGS) $(CXXFLAGS) $(OBJS_NATIVE) -o $(BUILD_DIR)/lox

debug: $(OBJS_DEBUG)
	g++ $(DEBUG_FLAGS) $(CXXFLAGS) $(OBJS_DEBUG) -o $(BUILD_DIR)/lox_debug

# Requires emscripten installed
web: $(OBJS_WEB)
	emcc $(WEB_FLAGS) $(CXXFLAGS) -sSTACK_SIZE=1048576 $(OBJS_WEB) -o $(BUILD_DIR)/lox.js \
		-sEXPORTED_FUNCTIONS=_lox_run_src,_init_vm,_malloc,_free -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
	cp web/index.html out/index.html
	cp web/ui.js out/ui.js
	cat out/lox.js web/worker.js > out/workerbundle.js

test: debug
	time python3 tests/test.py

bench: native
	time python3 tests/bench.py

all: native debug web

clean:
	$(RM) -r $(BUILD_DIR)

.PHONY: clean web native all test debug
