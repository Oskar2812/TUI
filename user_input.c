#include "../include/osk_tui.h"

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <wchar.h>
#include <stdlib.h>

void toggleEcho(Console* console, Toggle toggle) {
    DWORD dwMode;

    GetConsoleMode(console->inConsole, &dwMode);

    switch (toggle) {
        case ECHO_OFF:
            dwMode &= ~ENABLE_ECHO_INPUT;
            break;
        case ECHO_ON:
            dwMode |= ENABLE_ECHO_INPUT;
            break;
        default:
            dwMode |= ENABLE_ECHO_INPUT;
            break;
    }

    SetConsoleMode(console->inConsole, dwMode);

    updateConsole(console);
}

int getUserInput(Console* console, Mode mode) {
    wchar_t newChar = getch();

    if (newChar == '\r' || newChar == '\n') {
        switch (mode) {
            case INTO_CONSOLE:
                insertConstColourLineIntoVisibleBuffer(console, console->input.input_buffer, FOREGROUND_WHITE);
                break;
            case INTO_HIDDEN:
                wcsncpy(console->input.hidden_buffer, console->input.input_buffer, console->screen.visible_width + 1);
                break;
            default:
                insertConstColourLineIntoVisibleBuffer(console, console->input.input_buffer, FOREGROUND_WHITE);
                break;
        }
        resetInputBuffer(console);
        clearUserInputFromScreen(console);

        return 1;
    }

    COORD cursorPos = getCursorPosition(console);

    if (newChar == '\b') {
        removeFromInputBuffer(console, cursorPos.X - 1);
        setCursorPosition(console, cursorPos.X - 1, cursorPos.Y);
        return 0;
    }

    if (newChar == 0) {
        wchar_t ch = getch();

        if (ch == 77 && cursorPos.X < console->input.input_size) {
            setCursorPosition(console, cursorPos.X + 1, cursorPos.Y);
        }
        else if (ch == 75) {
            setCursorPosition(console, cursorPos.X - 1, cursorPos.Y);
        }

        return 0;
    }

    if (console->input.input_size == console->input.max_width) {
        switch (mode) {
            case INTO_CONSOLE:
                insertConstColourLineIntoVisibleBuffer(console, console->input.input_buffer, FOREGROUND_WHITE);
                break;
            case INTO_HIDDEN:
                wcscpy(console->input.hidden_buffer, console->input.input_buffer);
                break;
            default:
                insertConstColourLineIntoVisibleBuffer(console, console->input.input_buffer, FOREGROUND_WHITE);
                break;
        }

        console->input.input_size = 0;
        wcscpy(console->input.input_buffer, L"");
        resetCursor(console);
        insertIntoInputBuffer(console, newChar, 0);

        return 1;
    }

    insertIntoInputBuffer(console, newChar, cursorPos.X);
    setCursorPosition(console, cursorPos.X + 1, cursorPos.Y);

    return 0;
}

void printVisibleBuffer(Console* console) {
    // FILE* log = fopen("log.txt", "a");
    // fprintf(log, "Starting printing visible buffer\n");

    toggleCursorVisibility(console, CURSOR_INVISIBLE);

    setCursorPosition(console, 0, 0);

    wchar_t format[11];

    int index = 0;
    wchar_t borderFormat[11];
    getFormatString(console->border.border_colour, format);

    wcscpy(borderFormat, format);

    wchar_t borderChar = console->border.border_character;
    for (int jj = 0; jj < console->csbi.srWindow.Right; jj++) {
        swprintf(&console->output_buffer[index], 10, borderFormat, borderChar);
        index += 9;
    }
    console->output_buffer[index] = L'\n';
    index += 1;

    for (int ii = 0; ii < console->screen.visible_height; ii++) {
        swprintf(&console->output_buffer[index], 10, borderFormat, borderChar);
        index += 9;
        swprintf(&console->output_buffer[index], 9, L"\033[37;40m", borderChar);
        index += 8;
        for (int jj = 0; jj < console->screen.visible_width; jj++) {
            getFormatString(console->screen.visible_screen_buffer[MOD(-ii + console->screen.current_row, console->screen.visible_height)].colour[jj], format);
            
            swprintf(&console->output_buffer[index], 10, format, console->screen.visible_screen_buffer[MOD(-ii + console->screen.current_row, console->screen.visible_height)].text[jj]);
            index += 9;
        }
        swprintf(&console->output_buffer[index], 10, borderFormat, borderChar);
        index += 9;
        console->output_buffer[index] = L'\n';
        index += 1;
    }

    for (int jj = 0; jj < console->csbi.srWindow.Right; jj++) {
        swprintf(&console->output_buffer[index], 10, borderFormat, borderChar);
        index += 9;
    }
    console->output_buffer[index] = L'\n';
    index += 1;

    console->output_buffer[index] = L'\0';

    wprintf(L"%s", console->output_buffer);

    resetCursor(console);
    toggleCursorVisibility(console, CURSOR_VISIBLE);
    updateConsole(console);

    // fprintf(log, "Ending printing\n");
    // fclose(log);
}

void clearUserInputFromScreen(Console* console) {
    toggleCursorVisibility(console, CURSOR_INVISIBLE);
    resetCursor(console);
    for (int jj = 1; jj <= console->screen.visible_width; jj++) {
        fputs(" ", stdout);
    }

    resetCursor(console);
    toggleCursorVisibility(console, CURSOR_VISIBLE);
    updateConsole(console);
}

void printUserInput(Console* console, WORD colour) {
    COORD cursorPos = getCursorPosition(console);
    clearUserInputFromScreen(console);
    setTextColour(console, colour);
    fputws(console->input.input_buffer, stdout);
    setCursorPosition(console, cursorPos.X, cursorPos.Y);
    updateConsole(console);
}

void insertLineIntoVisibleBuffer(Console* console, TextData text) {
    wcsncpy(console->screen.visible_screen_buffer[console->screen.current_row].text, text.text, console->screen.visible_width);
    console->screen.visible_screen_buffer[console->screen.current_row].text[console->screen.visible_width] = '\0';

    memcpy(console->screen.visible_screen_buffer[console->screen.current_row].colour, &text.colour, sizeof(Colour) * console->screen.visible_width);

    console->screen.current_row = MOD(console->screen.current_row - 1, console->screen.visible_height);
}

void setMaxWidth(Console* console, int width) {
    console->input.max_width = width;
}

COORD getCursorPosition(Console* console) {
    updateConsole(console);

    return console->csbi.dwCursorPosition;
}

void insertIntoInputBuffer(Console* console, wchar_t newChar, int position) {
    if (position > console->input.max_width || position > console->input.input_size) {
        return;
    }

    if (position == console->input.input_size) {
        console->input.input_buffer[position] = newChar;
        console->input.input_size++;
    }
    else {
        for (int ii = (console->input.input_size > console->screen.visible_width - 1) ? console->input.input_size : console->screen.visible_width - 1; 
             ii >= position;
             ii--) 
        {
            console->input.input_buffer[ii + 1] = console->input.input_buffer[ii];
        }
        console->input.input_buffer[position] = newChar;
        console->input.input_size++;
    }
}

void removeFromInputBuffer(Console* console, int position) {
    if (position > console->input.max_width || position > console->input.input_size) {
        return;
    }
    for (int ii = position; ii < console->screen.visible_width - 1; ii++) {
            console->input.input_buffer[ii] = console->input.input_buffer[ii + 1];
    }

    console->input.input_size--;
}

void resetInputBuffer(Console* console) {
    wmemset(console->input.input_buffer, L' ', console->screen.visible_width);

    console->input.input_buffer[console->screen.visible_width + 1] = L'\0';

    console->input.input_size = 0;
}

int getColourCode(short colourValue) {
    switch (colourValue) {
        case 7:
            return 37;
        case 15:
            return 97;
        case 0:
            return 30;
        case 1:
            return 34;
        case 2:
            return 32;
        case 3:
            return 34;
        case 4:
            return 31;
        case 5:
            return 35;
        case 6:
            return 33;
        case 8:
            return 90;
        case 9:
            return 94;
        case 10:
            return 92;
        case 11:
            return 94;
        case 12:
            return 91;
        case 13:
            return 95;
        case 14:
            return 93;
        default:
            return 30;
    }
}

void getFormatString(Colour colour, wchar_t* result) {
    int foregroundColour, backgroundColour;

    foregroundColour = getColourCode(colour & 0xF);
    backgroundColour = getColourCode((colour >> 4) & 0xF) + 10;

    swprintf(result, 9,L"\033[%d;%dm", foregroundColour, backgroundColour);
    wcscat(result, L"%lc");
}