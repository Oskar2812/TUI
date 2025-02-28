#include "../include/osk_tui.h"
#include "../include/osk_tui_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

Console initConsole() {
    Console console;
    console.outConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console.outConsole == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        printf("Error: Getting the output handle failed with code: %lu\n", error);
        exit(1);
    }
    console.inConsole = GetStdHandle(STD_INPUT_HANDLE);
    if (console.inConsole == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        printf("Error: Getting the input handle failed with code: %lu\n", error);
        exit(1);
    }
    if (GetConsoleScreenBufferInfo(console.outConsole, &console.csbi) == 0) {
        DWORD error = GetLastError();
        printf("Error: Failed to gt console screen buffer info with code: %lu\n", error);
        exit(1);
    }

    console.screen.visible_height = console.csbi.srWindow.Bottom - 2;
    console.screen.visible_width = console.csbi.srWindow.Right - 2;
    if (console.screen.visible_height < 1 || console.screen.visible_width < 0) {
        #ifndef NDEBUG
            printf("Error: Console is too small please resize\n");
            exit(1);
        #endif

        console.screen.visible_height = 30;
        console.screen.visible_width = 30;
    }

    console.screen.current_row = console.screen.visible_height - 1;
    console.input.input_size = 0;

    console.screen.visible_screen_buffer = (TextData*)malloc(sizeof(TextData) * (console.screen.visible_height));
    if (console.screen.visible_screen_buffer == NULL) {
        printf("Error: Failed to allocate visible screen buffer\n");
        exit(1);
    }
    for (int ii = console.screen.visible_height - 1; ii >= 0; ii--) {
        console.screen.visible_screen_buffer[ii].text = (wchar_t*)malloc(sizeof(wchar_t) * (console.screen.visible_width + 1));
        console.screen.visible_screen_buffer[ii].colour = (Colour*)malloc(sizeof(Colour) * (console.screen.visible_width));
        if (console.screen.visible_screen_buffer[ii].text == NULL || console.screen.visible_screen_buffer[ii].colour == NULL) {
            printf("Error: Failed to allocate memory for row %d of the visible screen buffer\n", ii);
            exit(1);
        }

        insertBlankLineIntoVisibleBuffer(&console);
    }

    console.output_buffer = (wchar_t*)malloc(sizeof(wchar_t) * (15 * (console.csbi.srWindow.Bottom * console.csbi.srWindow.Right) + 15 * console.screen.visible_height  + console.csbi.srWindow.Bottom + 1));
    if (console.output_buffer == NULL) {
        printf("Error: Failed to allocate memory for output buffer");
        exit(1);
    }
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

    toggleVirtualProcessing(&console, VIRTUAL_PROCESSING_ON);

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
    if (!SetConsoleTextAttribute(hConsole, colour)) {
        DWORD error = GetLastError();
        printf("Error: Setting text colour failed with code: %lu\n", error);
    }
    updateConsole(console);
}

void setCursorPosition(Console* console, int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;

    if (!SetConsoleCursorPosition(console->outConsole, coord)) {
        DWORD error = GetLastError();
        printf("Failed to set cursor position with code: %lu\n", error);
    }
    updateConsole(console);
}

void clearConsole(Console* console) {
    DWORD dwSize = console->csbi.dwSize.X * console->csbi.dwSize.Y;
    DWORD dwWritten;

    if (!FillConsoleOutputCharacter(console->outConsole, ' ', dwSize, ZERO_COORD, &dwWritten) || !FillConsoleOutputAttribute(console->outConsole, console->csbi.wAttributes, dwSize, ZERO_COORD, &dwWritten)) {
        DWORD error = GetLastError();
        printf("Error: Failed to clear console with code: %lu\n", error);
        exit(1);
    }
    
    resetCursor(console);
    updateConsole(console);
}

void setBorder(Console* console, wchar_t border_character, WORD colour) {
    console->border.border_character = border_character;
    console->border.border_colour = colour;
}

void toggleCursorVisibility(Console* console, CursorToggle toggle) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(console->outConsole, &cursorInfo);

    switch (toggle) {
        case CURSOR_VISIBLE:
            cursorInfo.bVisible = TRUE;
            break;
        case CURSOR_INVISIBLE:
            cursorInfo.bVisible = FALSE;
            break;
    }

    if(!SetConsoleCursorInfo(console->outConsole, &cursorInfo)) {
        DWORD error = GetLastError();
        printf("Error: Failed to set cursos visibility with code: %lu\n", error);
        exit(1);
    }
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

void toggleVirtualProcessing(Console* console, ProcessingToggle toggle) {
    DWORD dwMode;

    if (!GetConsoleMode(console->outConsole, &dwMode)) {
        printf("Error: Failed to get console mode");
        exit(1);
    }

    switch (toggle) {
        case VIRTUAL_PROCESSING_ON:
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            break;
        case VIRTUAL_PROCESSING_OFF:
            dwMode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            break;
    }

    if (!SetConsoleMode(console->outConsole, dwMode)) {
        printf("Error: Failed to set console mode");
        exit(1);
    }

    return;
}