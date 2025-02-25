#include "../include/osk_tui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

Console initConsole() {
    Console console;
    console.outConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    console.inConsole = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleScreenBufferInfo(console.outConsole, &console.csbi);

    console.screen.visible_height = console.csbi.srWindow.Bottom - 2 <= 0 ? 30 : console.csbi.srWindow.Bottom - 2;
    console.screen.visible_width = console.csbi.srWindow.Right - 2 <= 0 ? 30 : console.csbi.srWindow.Right - 2;

    console.screen.current_row = console.screen.visible_height - 1;
    console.input.input_size = 0;

    console.screen.visible_screen_buffer = (TextData*)malloc(sizeof(TextData) * (console.screen.visible_height));
    for (int ii = console.screen.visible_height - 1; ii >= 0; ii--) {
        console.screen.visible_screen_buffer[ii].text = (wchar_t*)malloc(sizeof(wchar_t) * (console.screen.visible_width + 1));
        console.screen.visible_screen_buffer[ii].colour = (Colour*)malloc(sizeof(Colour) * (console.screen.visible_width));

        insertBlankLineIntoVisibleBuffer(&console);
    }

    // console.output_buffer = (wchar_t*)malloc(sizeof(wchar_t) * (9 * (console.csbi.srWindow.Bottom * console.csbi.srWindow.Right) + 8 * console.screen.visible_height  + console.csbi.srWindow.Bottom + 1));
    console.output_buffer = (wchar_t*)malloc(sizeof(wchar_t) * 10000);
    console.input.input_buffer = (wchar_t*)malloc(sizeof(wchar_t) * (console.screen.visible_width + 1));
    console.input.hidden_buffer = (wchar_t*)malloc(sizeof(wchar_t) * (console.screen.visible_width + 1)); 

    if (!console.input.input_buffer || !console.input.hidden_buffer) {
        wprintf(L"Memory allocation failed for input buffers.\n");
        exit(1);
    }

    wmemset(console.input.input_buffer, L' ', console.screen.visible_width);
    wmemset(console.input.hidden_buffer, L' ', console.screen.visible_width);

    console.input.input_buffer[console.screen.visible_width] = L'\0'; // line 36
    console.input.hidden_buffer[console.screen.visible_width] = L'\0';

    console.input.max_width = console.screen.visible_width;

    console.input.cursor_position = 0;

    console.border.border_character = ' ';
    console.border.border_colour = ALL_BLACK;

    return console;
}

void freeConsole(Console* console) {
    for (int ii = 0; ii < console->screen.visible_height; ii++) {
        free(console->screen.visible_screen_buffer[ii].text);
        free(console->screen.visible_screen_buffer[ii].colour);
    }

    free(console->screen.visible_screen_buffer);
    free(console->input.input_buffer);
    free(console->input.hidden_buffer);
    free(console->output_buffer);
}

void getConsoleData(Console* console) {
    printf("Size %d %d\n", console->csbi.dwSize.X, console->csbi.dwSize.Y);
    printf("Cursor %d %d\n", console->csbi.dwCursorPosition.X, console->csbi.dwCursorPosition.Y);
    printf("Atributes %d\n", console->csbi.wAttributes);
    printf("Scroll window %d %d\n", console->csbi.srWindow.Top, console->csbi.srWindow.Bottom);
    printf("Max Size %d %d\n", console->csbi.dwMaximumWindowSize.X, console->csbi.dwMaximumWindowSize.Y);
}

void updateConsole(Console* console) {
    GetConsoleScreenBufferInfo(console->outConsole, &console->csbi);
}

void setTextColour(Console* console, Colour colour) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, colour);
    updateConsole(console);
}

void setCursorPosition(Console* console, int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleCursorPosition(hConsole, coord);
    updateConsole(console);
}

void clearConsole(Console* console) {
    DWORD dwSize = console->csbi.dwSize.X * console->csbi.dwSize.Y;
    DWORD dwWritten;

    FillConsoleOutputCharacter(console->outConsole, ' ', dwSize, ZERO_COORD, &dwWritten);
    FillConsoleOutputAttribute(console->outConsole, console->csbi.wAttributes, dwSize, ZERO_COORD, &dwWritten);
    
    resetCursor(console);
    updateConsole(console);
}

void clearScreen(Console* console) {

    toggleCursorVisibility(console, CURSOR_INVISIBLE);

    wchar_t* temp = (wchar_t*)malloc(sizeof(wchar_t) * console->screen.visible_width - 1);
    if (temp == NULL) {
        printf("\033[37;40mTemp in clearScreen() was not allocated");
        exit(1);
    }
    wmemset(temp, L' ', console->screen.visible_width - 2);
    temp[console->screen.visible_width - 2] = L'\0';

    for (int ii = 1; ii <= console->screen.visible_height; ii++) {
        setCursorPosition(console, 1, ii);
        fputws(temp, stdout);

    }
    toggleCursorVisibility(console, CURSOR_VISIBLE);

    resetCursor(console);
    updateConsole(console);

    free(temp);
}

void setBorder(Console* console, wchar_t border_character, WORD colour) {

    console->border.border_character = border_character;
    console->border.border_colour = colour;
}

void toggleCursorVisibility(Console* console, Toggle toggle) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(console->outConsole, &cursorInfo);

    switch (toggle) {
        case CURSOR_VISIBLE:
            cursorInfo.bVisible = TRUE;
            break;
        case CURSOR_INVISIBLE:
            cursorInfo.bVisible = FALSE;
            break;
        default:
            cursorInfo.bVisible = TRUE;
            break;
    }

    SetConsoleCursorInfo(console->outConsole, &cursorInfo);
}

void resetCursor(Console* console) {
    setCursorPosition(console, 0, console->csbi.srWindow.Bottom);
}

void insertBlankLineIntoVisibleBuffer(Console* console) {
    
    wmemset(console->screen.visible_screen_buffer[console->screen.current_row].text, L' ', console->screen.visible_width);
    console->screen.visible_screen_buffer[console->screen.current_row].text[console->screen.visible_width] = L'\0';

    for (int ii = 0; ii < console->screen.visible_width; ii ++) {
        console->screen.visible_screen_buffer[console->screen.current_row].colour[ii] = FOREGROUND_BLACK | BACKGROUND_BLACK;
    }

    console->screen.current_row = MOD(console->screen.current_row - 1, console->screen.visible_height);
}

void insertConstColourLineIntoVisibleBuffer(Console* console, wchar_t* text, Colour colour) {
    wcsncpy(console->screen.visible_screen_buffer[console->screen.current_row].text, text, console->screen.visible_width);
    console->screen.visible_screen_buffer[console->screen.current_row].text[console->screen.visible_width] = '\0';

    for (int ii = 0; ii < console->screen.visible_width; ii ++) {
        console->screen.visible_screen_buffer[console->screen.current_row].colour[ii] = colour;
    }

    console->screen.current_row = MOD(console->screen.current_row - 1, console->screen.visible_height);
}

void clearVisibleBuffer(Console* console) {
    for (int ii = 0; ii < console->screen.visible_height; ii++) {
        insertBlankLineIntoVisibleBuffer(console);
    }
}