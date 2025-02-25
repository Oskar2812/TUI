#include "../include/osk_tui.h"

#include <stdio.h>
#include <conio.h>
#include <stdio.h>

void moving_circle(Console* console, int frame) {
    clearVisibleBuffer(console);
    insertCircle(console, frame, 6, 5, BACKGROUND_GREEN);
    insertCircle(console, console->screen.visible_width - frame, 6, 5, BACKGROUND_BLUE);
}

void do_nothing(Console* console, int frame) {
    clearVisibleBuffer(console);
}

int main() {
    Console console = initConsole();

    clearConsole(&console);

    //toggleEcho(&console, ECHO_OFF);

    // setBorder(&console, L'#', FOREGROUND_RED);

    // while(1) {
    //     if (kbhit()) {
    //         if (getUserInput(&console, INTO_CONSOLE)) {
    //             printVisibleBuffer(&console);
    //             printUserInput(&console, FOREGROUND_WHITE);
    //         }
    //         else {
    //             printUserInput(&console, FOREGROUND_WHITE);
    //         }
    //     }
    // }

    // setBorder(&console, L'#', ALL_WHITE);
    // printVisibleBuffer(&console);

    // animateLoop(&console, console.screen.visible_width, moving_circle);

    // insertCircle(&console, 50, 7, 5);
    // printVisibleBuffer(&console);

    Gif gif = processGif("C:\\Workspaces\\Oskar Grewe\\Extra Tasks\\TUI\\Gifs\\traffic_light.gif");

    playGif(&console, &gif);

    freeGif(&gif);
    freeConsole(&console);

    return 0;
}