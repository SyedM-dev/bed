SRC_DIR := src
BIN_DIR := bin
OBJ_DIR := build
INCLUDE_DIR := include

CXX ?= g++
CC ?= gcc

TARGET_DEBUG := $(BIN_DIR)/crib-dbg
TARGET_RELEASE := $(BIN_DIR)/crib

PCH_DIR_DEBUG := $(OBJ_DIR)/debug/pch
PCH_DIR_RELEASE := $(OBJ_DIR)/release/pch

PCH_DEBUG := $(PCH_DIR_DEBUG)/pch.h.gch
PCH_RELEASE := $(PCH_DIR_RELEASE)/pch.h.gch

PCRE_CFLAGS := $(shell pkg-config --cflags libpcre2-8)
PCRE_LIBS := $(shell pkg-config --static --libs libpcre2-8)

LIBGRAPHEME_CFLAGS := $(shell pkg-config --cflags libgrapheme)
LIBGRAPHEME_LIBS := $(shell pkg-config --static --libs libgrapheme)

MRUBY_CFLAGS ?= -I./libs/mruby/build/host/include
MRUBY_LIBS ?= -L./libs/mruby/build/host/lib -lmruby

CFLAGS_DEBUG :=\
	-std=c++26 -Wall -Wextra -static \
	-O0 -fno-inline -gsplit-dwarf \
	-g -fno-omit-frame-pointer -ffast-math \
	-I./include -I./libs/unicode_width

CFLAGS_RELEASE :=\
	-std=c++26 -O3 -march=x86-64 -mtune=generic -fno-rtti \
	-fvisibility=hidden -static -ffast-math \
	-fomit-frame-pointer -DNDEBUG -s \
	-I./include -I./libs/unicode_width

CFLAGS_DEBUG += $(PCRE_CFLAGS) $(LIBGRAPHEME_CFLAGS) $(MRUBY_CFLAGS) $(C_SANITIZER)
CFLAGS_RELEASE += $(PCRE_CFLAGS) $(LIBGRAPHEME_CFLAGS) $(MRUBY_CFLAGS)

UNICODE_SRC := $(wildcard libs/unicode_width/*.c)
UNICODE_OBJ := $(patsubst libs/unicode_width/%.c,$(OBJ_DIR)/unicode_width/%.o,$(UNICODE_SRC))

LIBS := $(PCRE_LIBS) $(LIBGRAPHEME_LIBS) $(MRUBY_LIBS)

SRC := $(shell find $(SRC_DIR) -type f -name '*.cc')
OBJ_DEBUG := $(patsubst $(SRC_DIR)/%.cc,$(OBJ_DIR)/debug/%.o,$(SRC))
OBJ_RELEASE := $(patsubst $(SRC_DIR)/%.cc,$(OBJ_DIR)/release/%.o,$(SRC))

DEP_DEBUG := $(OBJ_DEBUG:.o=.d)
DEP_RELEASE := $(OBJ_RELEASE:.o=.d)

.PHONY: all debug release clean

all: release

debug: $(TARGET_DEBUG)

release: $(TARGET_RELEASE)

$(PCH_DEBUG): $(INCLUDE_DIR)/pch.h
	@mkdir -p $(PCH_DIR_DEBUG)
	@$(CXX) $(CFLAGS_DEBUG) -x c++-header -o $@ $(INCLUDE_DIR)/pch.h
	@echo "Debug Precompiled header generated at $@"

$(PCH_RELEASE): $(INCLUDE_DIR)/pch.h
	@mkdir -p $(PCH_DIR_RELEASE)
	@$(CXX) $(CFLAGS_RELEASE) -x c++-header -o $@ $(INCLUDE_DIR)/pch.h
	@echo "Release Precompiled header generated at $@"

$(TARGET_DEBUG): $(PCH_DEBUG) $(OBJ_DEBUG) $(UNICODE_OBJ)
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CFLAGS_DEBUG) -o $@ $(OBJ_DEBUG) $(UNICODE_OBJ) $(LIBS)
	@echo "Debug build complete: $@"

$(TARGET_RELEASE): $(PCH_RELEASE) $(OBJ_RELEASE) $(UNICODE_OBJ)
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CFLAGS_RELEASE) -o $@ $(OBJ_RELEASE) $(UNICODE_OBJ) $(LIBS)
	@echo "Release build complete: $@"

$(OBJ_DIR)/debug/%.o: $(SRC_DIR)/%.cc $(PCH_DEBUG) $(GENERATED_HEADER)
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS_DEBUG) -I$(PCH_DIR_DEBUG) -include pch.h -MMD -MP -c $< -o $@
	@echo "[CXX] Debug object file generated: $@ from $<"

$(OBJ_DIR)/release/%.o: $(SRC_DIR)/%.cc $(PCH_RELEASE) $(GENERATED_HEADER)
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS_RELEASE) -I$(PCH_DIR_RELEASE) -include pch.h -MMD -MP -c $< -o $@
	@echo "[CXX] release object file generated: $@ from $<"

$(OBJ_DIR)/unicode_width/%.o: libs/unicode_width/%.c
	@mkdir -p $(dir $@)
	@$(CC) -MMD -MP -c $< -o $@
	@echo "[CC] Object file generated: $@ from $<"

DEP_DEBUG += $(UNICODE_OBJ:.o=.d)
DEP_RELEASE += $(UNICODE_OBJ:.o=.d)

-include $(DEP_DEBUG)
-include $(DEP_RELEASE)

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned build artifacts."
