#include "../../include/osk_tui.h"

#include <stdio.h>
#include <conio.h>
#include <stdio.h>

int main() {
    Console console = initConsole();

    clearConsole(&console);

    Gif gif = processGif("C:\\Workspaces\\OskarGrewe\\Extra Tasks\\TUI\\Gifs\\traffic_light.gif");

    playGif(&console, &gif);

    freeGif(&gif);
    freeConsole(&console);

    return 0;
}