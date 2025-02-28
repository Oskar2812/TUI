#include "../../include/osk_tui.h"

#include <stdio.h>
#include <conio.h>
#include <stdio.h>

void moving_circle(Console* console, int frame) {
    clearVisibleBuffer(console);
    insertCircle(console, frame, 6, 5, BACKGROUND_GREEN);
    insertCircle(console, console->screen.visible_width - frame, 6, 5, BACKGROUND_BLUE);
}

int main() {
    Console console = initConsole();

    clearConsole(&console);

    animateLoop(&console, console.screen.visible_width, moving_circle);

    freeConsole(&console);

    return 0;
}