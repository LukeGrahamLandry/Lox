BUILD_DIR ?= ./out
SRC_DIRS ?= ./src

SRCS_NATIVE := $(shell find $(SRC_DIRS) -name *.cc -and -not -name web.cc)
SRCS_WEB := $(shell find $(SRC_DIRS) -name *.cc -and -not -name main.cc)
OBJS_NATIVE := $(SRCS_NATIVE:%=$(BUILD_DIR)/native/%.o)
OBJS_WEB := $(SRCS_WEB:%=$(BUILD_DIR)/web/%.o)

# TODO: this is slow. have seperate debug/release options.
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security -fsanitize=undefined

DEPS := $(OBJS_NATIVE:.o=.d)
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
-include $(DEPS)

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -std=c++17

$(BUILD_DIR)/native/%.cc.o: %.cc
	mkdir -p $(dir $@)
	g++ $(ASAN_FLAGS) $(CPPFLAGS) -g $(CXXFLAGS) -c $< -o $@

# TODO: no -g for web because it makes the binary super big
$(BUILD_DIR)/web/%.cc.o: %.cc
	mkdir -p $(dir $@)
	emcc $(CPPFLAGS) -g -c $< -o $@

native: $(OBJS_NATIVE)
	g++ $(ASAN_FLAGS) $(CPPFLAGS) -g $(OBJS_NATIVE) -o $(BUILD_DIR)/lox

# Requires emscripten installed
web: $(OBJS_WEB)
	emcc $(CPPFLAGS) -g -sSTACK_SIZE=1048576 $(OBJS_WEB) -o $(BUILD_DIR)/lox.js \
		-sEXPORTED_FUNCTIONS=_lox_run_src,_init_vm,_malloc,_free -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
	cp web/index.html out/index.html
	cp web/ui.js out/ui.js
	cat out/lox.js web/worker.js > out/workerbundle.js

tests: native tests/tests.cc
	g++ $(ASAN_FLAGS) $(CPPFLAGS) -g tests/tests.cc -o $(BUILD_DIR)/tests

all: native web tests

clean:
	$(RM) -r $(BUILD_DIR)

.PHONY: clean web native all tests
