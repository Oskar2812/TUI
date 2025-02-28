typedef enum {
    VIRTUAL_PROCESSING_ON,
    VIRTUAL_PROCESSING_OFF,
} ProcessingToggle;

typedef enum {
    ECHO_ON,
    ECHO_OFF,
} EchoToggle;

void updateConsole(Console* console);

void resetCursor(Console* console);

void insertLineIntoVisibleBuffer(Console* console, TextData text);

void insertBlankLineIntoVisibleBuffer(Console* console);

void insertConstColourLineIntoVisibleBuffer(Console* console, wchar_t* text, Colour colour);

void toggleVirtualProcessing(Console* console, ProcessingToggle toggle);

void toggleEcho(Console* console, EchoToggle toggle);

void setMaxWidth(Console* console, int width);

COORD getCursorPosition(Console* console);

void insertIntoInputBuffer(Console* console, wchar_t text, int position);

void removeFromInputBuffer(Console* console, int position);

void resetInputBuffer(Console* console);

void getFormatString(Colour colour, wchar_t* result);

void getRGBFormatString(RGBColour colour, wchar_t* result);