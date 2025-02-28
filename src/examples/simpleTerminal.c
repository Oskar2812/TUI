#include "../../include/osk_tui.h"

#include <stdio.h>
#include <conio.h>
#include <stdio.h>

int main() {
    Console console = initConsole();

    clearConsole(&console);

    setBorder(&console, L'#', FOREGROUND_RED);

    while(1) {
        if (kbhit()) {
            if (getUserInput(&console, INTO_CONSOLE)) {
                printVisibleBuffer(&console);
                printUserInput(&console, FOREGROUND_WHITE);
            }
            else {
                printUserInput(&console, FOREGROUND_WHITE);
            }
        }
    }

    freeConsole(&console);

    return 0;
}