default: lib

CC      := gcc
CXX 	:= g++
CFLAGS  := -g -Wall -fPIC -DSM64_LIB_EXPORT -DVERSION_US -DNO_SEGMENTED_MEMORY -DGBI_FLOATS
LDFLAGS := -lm -shared -lpthread
ENDFLAGS := -fPIC
ifeq ($(OS),Windows_NT)
LDFLAGS := $(LDFLAGS) -mwindows
ENDFLAGS := -static -lole32 -lstdc++
endif

SRC_DIRS  := src src/decomp src/decomp/engine src/decomp/include/PR src/decomp/game src/decomp/pc src/decomp/pc/audio src/decomp/mario src/decomp/tools src/decomp/audio
BUILD_DIR := build
DIST_DIR  := dist
ifeq ($(BUILD_32),1)
  DIST_DIR  := dist32
  BUILD_DIR := build32
endif
ALL_DIRS  := $(addprefix $(BUILD_DIR)/,$(SRC_DIRS))

LIB_FILE   := $(DIST_DIR)/libsm64.so
LIB_H_FILE := $(DIST_DIR)/include/libsm64.h
TEST_FILE  := run-test

H_IMPORTED := $(C_IMPORTED:.c=.h)
IMPORTED   := $(C_IMPORTED) $(H_IMPORTED)

C_FILES   := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c)) $(C_IMPORTED)
CXX_FILES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
O_FILES   := $(foreach file,$(C_FILES),$(BUILD_DIR)/$(file:.c=.o)) $(foreach file,$(CXX_FILES),$(BUILD_DIR)/$(file:.cpp=.o))
DEP_FILES := $(O_FILES:.o=.d)

TEST_SRCS := test/main.c test/context.c test/level.c
TEST_OBJS := $(foreach file,$(TEST_SRCS),$(BUILD_DIR)/$(file:.c=.o))

ifeq ($(OS),Windows_NT)
  TEST_FILE := $(DIST_DIR)/$(TEST_FILE)
  LIB_FILE := $(DIST_DIR)/sm64.dll
endif

DUMMY != mkdir -p $(ALL_DIRS) build/test src/decomp/mario $(DIST_DIR)/include 


$(BUILD_DIR)/%.o: %.c $(IMPORTED)
	@$(CC) $(CFLAGS) -MM -MP -MT $@ -MF $(BUILD_DIR)/$*.d $<
	$(CC) -c $(CFLAGS) -isystem src/decomp/include -o $@ $<

$(BUILD_DIR)/%.o: %.cpp $(IMPORTED)
	@$(CXX) $(CFLAGS) -MM -MP -MT $@ -MF $(BUILD_DIR)/$*.d $<
	$(CXX) -c $(CFLAGS) -isystem src/decomp/include -o $@ $<

$(LIB_FILE): $(O_FILES)
	$(CC) $(LDFLAGS) -o $@ $^ $(ENDFLAGS)

$(LIB_H_FILE): src/libsm64.h
	cp -f $< $@


test/level.c test/level.h: ./import-test-collision.py
	./import-test-collision.py

test/main.c: test/level.h

$(BUILD_DIR)/test/%.o: test/%.c
	@$(CC) $(CFLAGS) -MM -MP -MT $@ -MF $(BUILD_DIR)/test/$*.d $<
ifeq ($(OS),Windows_NT)
	$(CC) -c $(CFLAGS) -I/mingw64/include/SDL2 -I/mingw64/include/GL -o $@ $<
	ifeq ($(BUILD_32),1)
		$(CC) -c $(CFLAGS) -I/mingw32/include/SDL2 -I/mingw32/include/GL -o $@ $<
	endif
else
	$(CC) -c $(CFLAGS) -o $@ $<
endif

$(TEST_FILE): $(LIB_FILE) $(TEST_OBJS)
ifeq ($(OS),Windows_NT)
	$(CC) -o $@ $(TEST_OBJS) $(LIB_FILE) `sdl2-config --cflags --libs` -lglew32 -lglu32 -lopengl32 -lSDL2 -lSDL2main -lm
else
	$(CC) -o $@ $(TEST_OBJS) $(LIB_FILE) -lGLEW -lGL -lSDL2 -lSDL2main -lm
endif


lib: $(LIB_FILE) $(LIB_H_FILE)

test: $(TEST_FILE) $(LIB_H_FILE)

run: test
	./$(TEST_FILE)

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR) test/level.? $(TEST_FILE)

-include $(DEP_FILES)
