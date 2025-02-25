#pragma once 

#include <windows.h>

#define ZERO_COORD (COORD){0 , 0}

#define FOREGROUND_WHITE FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define BACKGROUND_WHITE BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE
#define ALL_WHITE FOREGROUND_WHITE | BACKGROUND_WHITE

#define FOREGROUND_BLACK 0x0
#define BACKGROUND_BLACK (0x0 << 4)
#define ALL_BLACK FOREGROUND_BLACK | BACKGROUND_BLACK

#define ECHO_ON 0
#define ECHO_OFF 1

#define CURSOR_VISIBLE 0
#define CURSOR_INVISIBLE 1

#define INTO_CONSOLE 0
#define INTO_HIDDEN 1

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

Console initConsole();

void freeConsole(Console* console);

void getConsoleData(Console* console);

void updateConsole(Console* console);

void setTextColour(Console* console, Colour colour);

void setCursorPosition(Console* console, int x, int y);

/// @brief Clears the whole console
/// @param console the console bobject
void clearConsole(Console* console);

/// @brief Clears the "screen" part of the console - basically everything except the border
/// @param console the console object
void clearScreen(Console* console);

void clearUserInputFromScreen(Console* console);

void setBorder(Console* console, wchar_t border_character, WORD colour);

void toggleEcho(Console* console, Toggle toggle);

void printVisibleBuffer(Console* console);

void printUserInput(Console* console, WORD colour);

int getUserInput(Console* console, Mode mode);

void toggleCursorVisibility(Console* console, Toggle toggle);

void resetCursor(Console* console);

void insertLineIntoVisibleBuffer(Console* console, TextData text);

void insertBlankLineIntoVisibleBuffer(Console* console);

void insertConstColourLineIntoVisibleBuffer(Console* console, wchar_t* text, Colour colour);

void setMaxWidth(Console* console, int width);

COORD getCursorPosition(Console* console);

void insertIntoInputBuffer(Console* console, wchar_t text, int position);

void removeFromInputBuffer(Console* console, int position);

void insertIntoVisibleBuffer(Console* console, int x, int y, wchar_t text, Colour colour);

void resetInputBuffer(Console* console);

void insertSolidCircle(Console* console, int x, int y, int r, Colour colour);

void insertCircle(Console* console, int x, int y, int r, Colour colour);

void animateLoop(Console* console, int loopFrames, void (*frame_gen)(Console*, int));

void clearVisibleBuffer(Console* console);

void getFormatString(Colour colour, wchar_t* result);

Gif processGif(const char* filename);

void freeGif(Gif* gif);

void playGif(Console* console, Gif* gif);

void getRGBFormatString(RGBColour colour, wchar_t* result);

