#include "../include/osk_tui.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

void insertIntoVisibleBuffer(Console* console, int x, int y, wchar_t text, Colour colour) {
    if (x < 0 || x >= console->screen.visible_width) {
        return;
    }
    if (y < 0 || y >= console->screen.visible_height) {
        return;
    }

    console->screen.visible_screen_buffer[y].text[x] = text;
    console->screen.visible_screen_buffer[y].colour[x] = colour;
}

void insertSolidCircle(Console* console, int x, int y, int r, Colour colour) {
    int r2 = r * r;
    for (int ii = y - r; ii <= y + r; ii++) {
        for (int jj = x - r; jj <= x + r; jj++) {
            int dist2 = (ii - y) * (ii - y) + (jj - x) * (jj - x);
            if (dist2 < r2 + 7) {
                insertIntoVisibleBuffer(console, jj, ii, L' ' , colour);
            }
        }
    }
}

void insertCircle(Console* console, int x, int y, int r, Colour colour) {
    int r2 = r * r;
    for (int ii = y - r; ii <= y + r; ii++) {
        for (int jj = x - r; jj <= x + r; jj++) {
            int dist2 = (ii - y) * (ii - y) + (jj - x) * (jj - x);
            if (abs(dist2 - r2) < 7) {
                insertIntoVisibleBuffer(console, jj, ii, L' ' , colour);
            }
        }
    }
}

void animateLoop(Console* console, int loopFrames, void (*frame_gen)(Console*, int)) {
    clearVisibleBuffer(console);
    while (1) {
        for (int tt = 0; tt <= loopFrames; tt++) {
            //FILE* log = fopen("log.txt", "a");
            //fprintf(log, "Frame: %d\n", tt);
            frame_gen(console, tt);
            printVisibleBuffer(console);
            //fclose(log);
        }
    }
}

void playGif(Console* console, Gif* gif) {
    // toggleCursorVisibility(console, CURSOR_INVISIBLE);

    setCursorPosition(console, 0, 0);

    wchar_t format[21];

    RGBColour* table;

    int strLen = wcslen(L"\033[48;2;000;000;000m ") + 1;

    clock_t startTime;
    double endTime;
    int firstTimeFlag = 1;
    while(1) {
        for (int ff = 0; ff < gif-> frameCount; ff++) {
            setCursorPosition(console, 0, 0);
            int gifCoord = 0;
            int index = 0;
            if (gif->frames[ff].lct.table == NULL) {
                table = gif->gct.table;
            }
            else {
                table = gif->frames[ff].lct.table;
            }
            for (int ii = 0; ii < gif->height; ii++) {
                for (int jj = 0; jj < gif->width; jj++) {
                    if ((jj < gif->frames[ff].data.left || jj >= gif->frames[ff].data.left + gif->frames[ff].data.width) 
                            || 
                        (ii < gif->frames[ff].data.top || ii >= gif->frames[ff].data.top + gif->frames[ff].data.height)
                    ) {
                        index += strLen - 1;
                        continue;
                    }
                    int colourIndex = gif->frames[ff].indexStream[gifCoord];
                    
                    wchar_t newString[30];
                    snwprintf(newString, strLen, L"\033[48;2;%03d;%03d;%03dm ", table[colourIndex].red, table[colourIndex].green, table[colourIndex].blue);
                    wcsncpy(&console->output_buffer[index], newString, strLen - 1);
                    index += strLen - 1;
                    gifCoord += 1;
                }
                swprintf(&console->output_buffer[index], strLen - 1, L"\033[48;2;000;000;000m");
                index += strLen - 2;
                console->output_buffer[index] = L'\n';
                index += 1;
            }

            console->output_buffer[index] = L'\n';
            index += 1;

            console->output_buffer[index] = L'\0';

            endTime = clock() - startTime;
            if (firstTimeFlag) {
                wprintf(L"%s", console->output_buffer);
                firstTimeFlag = 0;
            }
            else if (gif->frames[MOD(ff - 1, gif->frameCount)].gc.delay < 100 * endTime) {
                wprintf(L"%s", console->output_buffer);
            }
            else {
                Sleep(10 * gif->frames[MOD(ff - 1, gif->frameCount)].gc.delay - 1000 * endTime);
                wprintf(L"%s", console->output_buffer);
            }
            startTime = clock();

            if (gif->frames[ff].gc.disposalMode == OVERWRITE_WITH_BACKGROUND) {
                getRGBFormatString(gif->gct.table[gif->gct.backgroundColour], format);
                for (int ii = index; ii > 0; ii--) {
                    swprintf(&console->output_buffer[index], 15, format, L' ');
                    index--;
                }
            }
        }
    }

    resetCursor(console);
    toggleCursorVisibility(console, CURSOR_VISIBLE);
    updateConsole(console);
}

void getRGBFormatString(RGBColour colour, wchar_t* result) {
    swprintf(result, 19, L"\033[48;2;%03d;%03d;%03dm", colour.red, colour.green, colour.blue);
    wcscat(result, L"%lc");
}