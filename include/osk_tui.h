#pragma once 

#include <windows.h>

#define ZERO_COORD (COORD){0 , 0}

#define FOREGROUND_WHITE FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define BACKGROUND_WHITE BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE
#define ALL_WHITE FOREGROUND_WHITE | BACKGROUND_WHITE

#define FOREGROUND_BLACK 0x0
#define BACKGROUND_BLACK (0x0 << 4)
#define ALL_BLACK FOREGROUND_BLACK | BACKGROUND_BLACK

#define DISPOSAL_NOT_SPECIFIED 0
#define DO_NOT_DISPOSE 1
#define OVERWRITE_WITH_BACKGROUND 2

#define MOD(dividend, divisor) \
    ((dividend) % (divisor) < 0 ? (dividend) % (divisor) + (divisor) : (dividend) % (divisor))

typedef short Toggle;

typedef short Mode;

typedef WORD Colour;

typedef struct {
    wchar_t* text;
    Colour* colour;
} TextData;

typedef struct {
    wchar_t border_character;
    Colour border_colour;
} Border;

typedef struct {
    TextData* visible_screen_buffer;
    int current_row;
    int visible_width;
    int visible_height;
} Screen;

typedef struct {
    wchar_t* input_buffer;
    wchar_t* hidden_buffer;
    int input_size;
    int max_width;
    int cursor_position;
} Input;

typedef struct {
    HANDLE outConsole;
    HANDLE inConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    Border border;
    Screen screen;
    Input input;
    wchar_t* output_buffer;
} Console;

typedef struct {
    BYTE red;
    BYTE green;
    BYTE blue;
} RGBColour;

typedef struct {
    unsigned int size;
    unsigned int backgroundColour;
    RGBColour* table;
} ColourTable;

typedef struct {
    WORD delay; // in a hundreth of a second
    BYTE colourIndex;
    Mode disposalMode;
} GraphicControl;

typedef struct {
    WORD left;
    WORD top;
    WORD width;
    WORD height;
} FrameData;

typedef struct {
    FrameData data;
    ColourTable lct;
    GraphicControl gc;
    int* indexStream;
} Frame;

typedef struct {
    WORD height;
    WORD width;
    BYTE lsd;
    ColourTable gct;
    GraphicControl currentGraphicControl;
    Frame* frames;
    int frameCount;
} Gif;

typedef struct {
    int length;
    int* indexes;
} CodeIndex;

typedef struct {
    CodeIndex* codeIndex;
    int* codes;
} CodeTable;

typedef enum {
    CURSOR_VISIBLE,
    CURSOR_INVISIBLE,
} CursorToggle;

typedef enum {
    INTO_CONSOLE,
    INTO_HIDDEN,
} InputMode;

/**
 * @brief Initializes the console for text interaction.
 *
 * This function sets up the console by obtaining input/output handles, 
 * retrieving console screen buffer info, and allocating memory for 
 * buffers used in rendering the console. It also ensures the console 
 * dimensions are valid and configures the necessary settings.
 *
 * @return Console A fully initialized `Console` structure.
 */
Console initConsole();

/**
 * @brief Frees the memory allocated for the console.
 *
 * This function deallocates all memory allocated for the console's 
 * screen buffer, input buffers, and output buffer, ensuring proper 
 * memory management and cleanup when the console is no longer needed.
 *
 * @param console A pointer to the `Console` structure to be freed.
 */
void freeConsole(Console* console);

/**
 * @brief Prints the current console's data.
 *
 * This function outputs various details about the console's current 
 * state, including size, cursor position, attributes, scroll window 
 * position, and maximum window size to the standard output.
 *
 * @param console A pointer to the `Console` structure containing the console data.
 */
void getConsoleData(Console* console);

/**
 * @brief Sets the text colour for the console output.
 *
 * This function changes the text colour of the console by using 
 * `SetConsoleTextAttribute` to set the console's text attributes 
 * to the specified colour. If the operation fails, an error message is printed.
 *
 * @param console A pointer to the `Console` structure which contains console settings.
 * @param colour The colour to set for the text.
 */
void setTextColour(Console* console, Colour colour);

/**
 * @brief Sets the cursor position in the console.
 *
 * This function moves the cursor to the specified (x, y) coordinates 
 * within the console window using `SetConsoleCursorPosition`. 
 * If the operation fails, an error message is printed.
 *
 * @param console A pointer to the `Console` structure which contains console settings.
 * @param x The horizontal position of the cursor.
 * @param y The vertical position of the cursor.
 */
void setCursorPosition(Console* console, int x, int y);

/**
 * @brief Clears the console screen.
 * 
 * This function clears the entire console screen by filling the console's output
 * with blank spaces and resetting the text attributes to the default values.
 * After clearing, it also resets the cursor to its initial position and updates
 * the console display.
 * 
 * @param console A pointer to the `Console` struct representing the console state.
 * 
 * @note If the operation fails, the program exits with an error message.
 */
void clearConsole(Console* console);

void clearUserInputFromScreen(Console* console);

/**
 * @brief Sets the border character and colour for the console.
 * 
 * This function updates the console's border character and its colour to the specified values.
 * It modifies the `border_character` and `border_colour` fields in the `Console` struct.
 * 
 * @param console A pointer to the `Console` struct representing the console state.
 * @param border_character The character to be used for the border.
 * @param colour The colour code to be applied to the border.
 */
void setBorder(Console* console, wchar_t border_character, WORD colour);

/**
 * @brief Prints the visible buffer content to the console.
 * 
 * This function renders the current visible screen buffer along with borders to the console output. 
 * It applies the border character and color, fills in the content of the screen buffer with the appropriate colors, 
 * and displays the entire output with proper formatting.
 * 
 * It temporarily hides the cursor during the rendering process and restores it once the output is displayed.
 * 
 * @param console A pointer to the `Console` struct representing the console state.
 * 
 * @return void
 */
void printVisibleBuffer(Console* console);

/**
 * @brief Prints the user input to the console with a specified text color.
 * 
 * This function clears any existing user input from the screen, sets the specified text color, 
 * and then prints the user input (stored in `input_buffer`) to the console. After printing, the 
 * cursor is restored to its original position.
 * 
 * @param console A pointer to the `Console` struct representing the console state.
 * @param colour The color to apply to the text when printing the user input.
 * 
 * @return void
 */
void printUserInput(Console* console, WORD colour);

/**
 * @brief Handles user input from the console and processes it based on the given mode.
 * 
 * This function reads a character from the console input and processes it. It handles special keys
 * like Enter (`\r` or `\n`), Backspace (`\b`), and Arrow keys (left and right). Based on the mode,
 * it either inserts the input into the visible console buffer or into a hidden buffer. If the input
 * exceeds the maximum width, it resets the input buffer and starts fresh.
 * 
 * @param console A pointer to the `Console` struct representing the console state.
 * @param mode The mode determining where to store the input (e.g., visible console or hidden buffer).
 * 
 * @return An integer indicating the status:
 *         - 1 if the input has been processed (e.g., Enter pressed),
 *         - 0 if the input is still being processed.
 */
int getUserInput(Console* console, InputMode mode);

/**
 * @brief Toggles the visibility of the console cursor.
 * 
 * @param console A pointer to the `Console` struct representing the console state.
 * @param toggle An enumeration value indicating whether to make the cursor visible or invisible.
 * 
 * @return void
 */
void toggleCursorVisibility(Console* console, CursorToggle toggle);

/**
 * @brief Inserts a solid circle into the visible buffer at the specified coordinates.
 * 
 * This function draws a solid circle in the console at the given `(x, y)` position with the specified 
 * radius `r` and color. The circle is inserted by calculating the distance from the center and checking 
 * if each point lies within the radius, considering a small tolerance for smoothing the edges.
 * 
 * @param console A pointer to the `Console` struct representing the console state.
 * @param x The x-coordinate of the center of the circle.
 * @param y The y-coordinate of the center of the circle.
 * @param r The radius of the circle.
 * @param colour The color to fill the circle.
 */
void insertSolidCircle(Console* console, int x, int y, int r, Colour colour);

/**
 * @brief Inserts a circle into the visible buffer at the specified coordinates.
 * 
 * This function draws a circle in the console at the given `(x, y)` position with the specified 
 * radius `r` and color. It draws the outline of the circle by calculating the distance from the 
 * center and checking if the distance is within a small tolerance (`7` units) of the radius.
 * 
 * @param console A pointer to the `Console` struct representing the console state.
 * @param x The x-coordinate of the center of the circle.
 * @param y The y-coordinate of the center of the circle.
 * @param r The radius of the circle.
 * @param colour The color to outline the circle.
 */
void insertCircle(Console* console, int x, int y, int r, Colour colour);

/**
 * @brief Animates a loop of frames and prints them to the console.
 * 
 * This function clears the visible buffer, then enters an infinite loop where it generates and 
 * displays frames one by one. It calls the provided `frame_gen` function to generate the frames 
 * and prints the updated visible buffer to the console after each frame.
 * 
 * @param console A pointer to the `Console` struct representing the console state.
 * @param loopFrames The number of frames to animate in one cycle.
 * @param frame_gen A function pointer to the frame generation function that takes in the `console` 
 *                  and the current frame index.
 */
void animateLoop(Console* console, int loopFrames, void (*frame_gen)(Console*, int));

/**
 * @brief Processes a GIF file and extracts its data into a Gif structure.
 * 
 * This function reads a GIF file and extracts its relevant properties, such as width, height, 
 * global color table, and individual frames. It processes the file in binary mode, handling 
 * extensions, frames, and the global color table (GCT).
 * 
 * @param filename The name of the GIF file to be processed.
 * @return A `Gif` struct containing the parsed data from the GIF file.
 */
Gif processGif(const char* filename);

/**
 * @brief Frees the memory allocated for a Gif structure.
 * 
 * This function frees the memory used by the global color table (GCT) and the local color 
 * tables (LCT) of all frames in the GIF, as well as the memory for the frames array itself.
 * 
 * @param gif A pointer to the `Gif` structure to be freed.
 */
void freeGif(Gif* gif);

/**
 * @brief Plays a GIF in the console, frame by frame.
 * 
 * This function renders a GIF to the console by cycling through each frame. It updates the console 
 * output with the appropriate colors from the GIF's palette and handles delays between frames based 
 * on the GIF's frame delays. It continuously loops through the frames until interrupted.
 * 
 * @param console A pointer to the `Console` struct representing the console state.
 * @param gif A pointer to the `Gif` struct containing the GIF data to be played.
 */
void playGif(Console* console, Gif* gif);

/**
 * @brief Clears the visible screen buffer by inserting blank lines.
 * 
 * This function iterates through the visible height of the console screen
 * and clears the content by inserting blank lines into the visible buffer.
 * It is typically used to reset or clear the screen before rendering new content.
 * 
 * @param console A pointer to the `Console` object, which contains the screen buffer 
 * and other relevant properties for rendering.
 */
void clearVisibleBuffer(Console* console);

/**
 * @brief Inserts a character with a specified colour into the visible screen buffer at a given position.
 * 
 * This function updates the visible screen buffer by placing the provided character (`text`) 
 * and colour (`colour`) at the specified coordinates (`x`, `y`). If the coordinates are out of bounds, 
 * the function does nothing to avoid accessing invalid memory.
 * 
 * @param console A pointer to the `Console` object that holds the screen buffer and related properties.
 * @param x The horizontal position (column) in the visible buffer where the character will be inserted.
 * @param y The vertical position (row) in the visible buffer where the character will be inserted.
 * @param text The character to be inserted at the specified position.
 * @param colour The colour to be applied to the character at the specified position.
 */
void insertIntoVisibleBuffer(Console* console, int x, int y, wchar_t text, Colour colour);