CLANG = clang

DEBUG_CFLAGS = -g -Wall -Werror -fsanitize=address
CFLAGS = -Wall -Werror

BUILD = build
SRC = src
EXAMPLES = $(SRC)/examples

TUI_LIB = libosk_tui.a
TUI_LIB_DEBUG = libosk_tui_debug.a  # Debug version of the library
LIB_OBJECTS = $(BUILD)/console_design.o $(BUILD)/user_input.o $(BUILD)/console_graphics.o $(BUILD)/external_files.o
LIB_OBJECTS_DEBUG = $(LIB_OBJECTS:.o=.o.debug)

# Default target
all: lib

$(BUILD):
	mkdir $(BUILD)

# Rule to compile object files for the regular build (without debug flags)
$(BUILD)/%.o: $(SRC)/%.c $(BUILD)
	$(CLANG) $(CFLAGS) -c $< -o $@

# Rule to compile object files for the debug build (with DEBUG_CFLAGS)
$(BUILD)/%.o.debug: $(SRC)/%.c
	$(CLANG) $(DEBUG_CFLAGS) -c $< -o $@

# Rule to create the regular static library (non-debug)
$(TUI_LIB): $(LIB_OBJECTS)
	ar rcs $@ $^

# Rule to create the debug static library (debug version)
$(TUI_LIB_DEBUG): $(LIB_OBJECTS_DEBUG)
	ar rcs $@ $^

# Target for building the test executable (non-debug)
test: $(BUILD)/test.o $(TUI_LIB)
	$(CLANG) $(CFLAGS) $(LINK_FLAGS) $^ -o $@

# Target for building the debug version of the test executable
debug: $(BUILD)/test.o.debug $(TUI_LIB_DEBUG)
	$(CLANG) $(DEBUG_CFLAGS) $^ -o test-debug

# Target for building the library (non-debug)
lib: $(TUI_LIB)

examples: $(TUI_LIB)
	$(CLANG) $(CFLAGS) -c $(EXAMPLES)/processGif.c -o $(BUILD)/processGif.o
	$(CLANG) $(CFLAGS) $(BUILD)/processGif.o $(TUI_LIB) -o processGif
	$(CLANG) $(CFLAGS) -c $(EXAMPLES)/simpleAnimation.c -o $(BUILD)/simpleAnimation.o
	$(CLANG) $(CFLAGS) $(BUILD)/simpleAnimation.o $(TUI_LIB) -o simpleAnimation
	$(CLANG) $(CFLAGS) -c $(EXAMPLES)/simpleTerminal.c -o $(BUILD)/simpleTerminal.o
	$(CLANG) $(CFLAGS) $(BUILD)/simpleTerminal.o $(TUI_LIB) -o simpleTerminal

# Clean up the build files
clean: 
	del /f /q build\*
	del /f /q *.exe
	del /f /q *.a

# Mark these targets as phony
.PHONY: all test clean lib debug examples