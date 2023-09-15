BUILD_DIR ?= ./out
SRC_DIRS ?= ./src

SRCS_NATIVE := $(shell find $(SRC_DIRS) -name *.cc -and -not -name web.cc)
SRCS_WEB := $(shell find $(SRC_DIRS) -name *.cc -and -not -name main.cc)
OBJS_NATIVE := $(SRCS_NATIVE:%=$(BUILD_DIR)/native/%.o)
OBJS_WEB := $(SRCS_WEB:%=$(BUILD_DIR)/web/%.o)

# this makes it cope with changes header files apparently?
DEPS := $(OBJS_NATIVE:.o=.d)
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
-include $(DEPS)

# TODO: -DCOMPILER_DEBUG_PRINT_CODE can pass variables to the preprocessor. use that for debug mode that enables checks or verbose mode that logs?
#       suddenly im having a combinatorial explosion that would benefit from using a build system that has functions.
CXXFLAGS := $(INC_FLAGS) -MMD -MP -std=c++17
# TODO: figure out if this is notably slower
ASAN_FLAGS := -fsanitize=address -fno-omit-frame-pointer -Wno-format-security -fsanitize=undefined

$(BUILD_DIR)/native/%.cc.o: %.cc
	mkdir -p $(dir $@)
	g++ $(ASAN_FLAGS) $(CXXFLAGS) -g -c $< -o $@

$(BUILD_DIR)/web/%.cc.o: %.cc
	mkdir -p $(dir $@)
	emcc $(CXXFLAGS) -c $< -o $@

native: $(OBJS_NATIVE)
	g++ $(ASAN_FLAGS) $(CXXFLAGS) -g $(OBJS_NATIVE) -o $(BUILD_DIR)/lox

# Requires emscripten installed
web: $(OBJS_WEB)
	emcc $(CXXFLAGS) -sSTACK_SIZE=1048576 $(OBJS_WEB) -o $(BUILD_DIR)/lox.js \
		-sEXPORTED_FUNCTIONS=_lox_run_src,_init_vm,_malloc,_free -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
	cp web/index.html out/index.html
	cp web/ui.js out/ui.js
	cat out/lox.js web/worker.js > out/workerbundle.js

test: native
	time python3 tests/test.py

all: native web

clean:
	$(RM) -r $(BUILD_DIR)

.PHONY: clean web native all test
