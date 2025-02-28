#include "../include/osk_tui.h"
#include "../include/osk_tui_internal.h"

#include <stdio.h>
#include <math.h>
#include <stdint.h>

void processExtension(Gif* gif, FILE* file) {
    BYTE label;
    fread(&label, 1, 1, file);
    BYTE temp[32];
    BYTE size;
    BYTE packed;
    switch (label) {
        case 0xff:
            fread(temp, 12, 1, file);
            fread(&size, 1, 1, file);
            fread(temp, size + 1, 1, file);
            break;
        case 0xf9:
            fread(temp, 1, 1, file);
            fread(&packed, 1, 1, file);
            gif->currentGraphicControl.disposalMode = (packed & 0b00011100) >> 2;
            fread(&gif->currentGraphicControl.delay, 2, 1, file);
            if ((packed & 0x1) == 1) {
                fread(&gif->currentGraphicControl.colourIndex, 1, 1, file);
            }
            else {
                gif->currentGraphicControl.colourIndex = -1;
                fread(temp, 1, 1, file);
            }
            fread(temp, 1, 1, file);
            break;
    }
}

CodeTable initialiseCodeTable(int codeSize) {
    CodeTable codeTable;

    CodeIndex* table = (CodeIndex*)malloc(sizeof(CodeIndex) * (1 << codeSize));
    codeTable.codes = (int*)calloc((1 << codeSize), sizeof(int));

    for (int ii = 0; ii < (1 << (codeSize - 1)); ii++) {
        table[ii].length = 1;
        table[ii].indexes = (int*)malloc((sizeof(int)));
        table[ii].indexes[0] = ii;
        codeTable.codes[ii] = ii;
    }

    table[1 << (codeSize - 1)].length = 0;
    table[1 << (codeSize - 1)].indexes = NULL;
    codeTable.codes[1 << (codeSize - 1)] = 1 << (codeSize - 1);

    table[(1 << (codeSize - 1)) + 1].length = 0;
    table[(1 << (codeSize - 1)) + 1].indexes = NULL;
    codeTable.codes[(1 << (codeSize - 1)) + 1] = (1 << (codeSize - 1)) + 1;

    codeTable.codeIndex = table;

    return codeTable;
}

void addCode(CodeTable* codeTable, int code, int* indexes, int length) {
    codeTable->codeIndex[code].indexes = (int*)malloc(sizeof(int) * length);
    memcpy(codeTable->codeIndex[code].indexes, indexes, sizeof(int) * length);
    codeTable->codeIndex[code].length = length;
    codeTable->codes[code] = code;
}

uint32_t extractBits(uint32_t num, int m, int n) {
    uint32_t block = 0;

    if (m + n <= 32) {
        block = (num >> m) & ((1U << n) - 1);
    } else {
        // Case 2: Block wraps around
        int bits_from_end = 32 - m;  // Number of bits from m to the end
        int bits_from_start = n - bits_from_end;  // Remaining bits to be extracted from the start

        // Extract bits from m to the end of the number
        block = (num >> m) & ((1U << bits_from_end) - 1);
        
        // Extract the remaining bits from the start of the number (wrap-around)
        block |= (num & ((1U << bits_from_start) - 1)) << bits_from_end;
    }

    return block;
}

uint32_t rotateRight(uint32_t num, uint32_t bits, uint32_t total_bits) {
    return (num >> bits) | (num << (total_bits - bits));
}

void decodeLZW(Gif* gif, FILE* file, int originalCodeSize, int clearCode) {
    int codeSize = originalCodeSize;
    CodeTable codeTable = initialiseCodeTable(originalCodeSize);
    int eoiCode = (1U << (codeSize - 1)) + 1;

    int frameSize = gif->frames[gif->frameCount].data.width * gif->frames[gif->frameCount].data.height;
    int streamPos = 0;

    BYTE blockSize;
    fread(&blockSize, 1, 1, file);

    uint32_t buffer;
    uint8_t bufferPos = 0;

    fread(&buffer, blockSize < 4 ? blockSize : 4, 1, file);
    blockSize -= blockSize < 4 ? blockSize : 4;

    uint16_t code = extractBits(buffer, bufferPos, codeSize);
    bufferPos = (bufferPos + codeSize) % 32;
    if (code != clearCode) {
        printf("Error: Incorrect Gif format, clear code not found at start of image data\n");
        exit(1);
    }

    code = extractBits(buffer, bufferPos, codeSize);
    bufferPos = (bufferPos + codeSize) % 32;
    if (code > (1U << (codeSize - 1))) {
        printf("Error: Incorrect Gif format, first code not in code table");
    }

    memcpy(&gif->frames[gif->frameCount].indexStream[streamPos], codeTable.codeIndex[code].indexes, sizeof(int) * codeTable.codeIndex[code].length);
    streamPos = streamPos + codeTable.codeIndex[code].length;

    uint16_t previousCode = code;
    uint16_t newCode = (1U << (codeSize - 1)) + 2;

    while (streamPos < frameSize) {
        while (1) {
            if (bufferPos >= 32 - codeSize) {
                fread(&buffer, blockSize < 3 ? blockSize : 3, 1, file);
                blockSize -= blockSize < 3 ? blockSize : 3;
                if (blockSize == 0) {
                    fread(&blockSize, 1, 1, file);
                }

                code = extractBits(buffer, bufferPos, codeSize);
                bufferPos = (bufferPos + codeSize) % 32;

                if (blockSize >= 1) {
                    buffer = buffer << 8;
                    fread(&buffer, 1, 1, file);
                    blockSize -= 1;
                    buffer = rotateRight(buffer, 8, 32);
                }
            }
            else {
                code = extractBits(buffer, bufferPos, codeSize);
                bufferPos = (bufferPos + codeSize) % 32;
            }

            if (code == eoiCode) {
                break;
            }
            else if (code == clearCode) {
                for (int ii = 0; ii < (1 << codeSize); ii++) {
                    if (codeTable.codes[ii] == ii) {
                        free(codeTable.codeIndex[ii].indexes);
                    }
                }

                free(codeTable.codeIndex);
                codeTable = initialiseCodeTable(originalCodeSize);
                codeSize = originalCodeSize;
                newCode = (1U << (codeSize - 1)) + 2;
                continue;
            }
            else {
                if (codeTable.codes[code] == code) {
                    memcpy(&gif->frames[gif->frameCount].indexStream[streamPos], codeTable.codeIndex[code].indexes, sizeof(int) * codeTable.codeIndex[code].length);
                    streamPos = streamPos + codeTable.codeIndex[code].length;

                    int k = codeTable.codeIndex[code].indexes[0];

                    int* indexes = (int*)malloc(sizeof(int) * (codeTable.codeIndex[previousCode].length + 1));
                    memcpy(indexes, codeTable.codeIndex[previousCode].indexes, sizeof(int) * codeTable.codeIndex[previousCode].length);
                    indexes[codeTable.codeIndex[previousCode].length] = k;

                    addCode(&codeTable, newCode, indexes, codeTable.codeIndex[previousCode].length + 1);
                    newCode++;
                    free(indexes);
                }
                else {
                    int k = codeTable.codeIndex[previousCode].indexes[0];

                    int* indexes = (int*)malloc(sizeof(int) * (codeTable.codeIndex[previousCode].length + 1));
                    memcpy(indexes, codeTable.codeIndex[previousCode].indexes, sizeof(int) * codeTable.codeIndex[previousCode].length);
                    indexes[codeTable.codeIndex[previousCode].length] = k;

                    memcpy(&gif->frames[gif->frameCount].indexStream[streamPos], indexes, sizeof(int) * (codeTable.codeIndex[previousCode].length + 1));
                    streamPos = streamPos + codeTable.codeIndex[previousCode].length + 1;

                    addCode(&codeTable, newCode, indexes, codeTable.codeIndex[previousCode].length + 1);
                    newCode++;
                    free(indexes);
                }

                previousCode = code;

                if (newCode == (1 << codeSize)) {
                    codeSize++;
                    codeTable.codeIndex = (CodeIndex*)realloc(codeTable.codeIndex, sizeof(CodeIndex) * (1 << codeSize));
                    codeTable.codes = (int*)realloc(codeTable.codes, sizeof(int) * (1 << codeSize));
                }
            }

        }
    }
    for (int ii = 0; ii < (1 << codeSize); ii++) {
        if (codeTable.codes[ii] == ii) {
            free(codeTable.codeIndex[ii].indexes);
        }
    }
    free(codeTable.codeIndex);
}

void processFrame(Gif* gif, FILE* file) {
    gif->frames = (Frame*)realloc(gif->frames, sizeof(Frame) * (gif->frameCount + 1));

    BYTE packed;

    gif->frames[gif->frameCount].gc = gif->currentGraphicControl;

    fread(&gif->frames[gif->frameCount].data.left, 2, 1, file);
    fread(&gif->frames[gif->frameCount].data.top, 2, 1, file);
    fread(&gif->frames[gif->frameCount].data.width, 2, 1, file);
    fread(&gif->frames[gif->frameCount].data.height, 2, 1, file);
    fread(&packed, 1, 1, file);

    gif->frames[gif->frameCount].indexStream = (int*)malloc(sizeof(int) * gif->frames[gif->frameCount].data.width * gif->frames[gif->frameCount].data.height);

    if ((packed & 0x1) == 1) {
        gif->frames[gif->frameCount].lct.size = 3L * (1L << ((packed & 0b11100000) + 1));

        BYTE* rawGlobalColourTable = (BYTE*)malloc(sizeof(BYTE) * gif->frames[gif->frameCount].lct.size);
        fread(rawGlobalColourTable, gif->frames[gif->frameCount].lct.size, 1, file);

        gif->frames[gif->frameCount].lct.table = (RGBColour*)malloc(sizeof(RGBColour) * gif->frames[gif->frameCount].lct.size);
        for (int ii = 0; ii < gif->frames[gif->frameCount].lct.size / 3; ii += 1) {
            gif->frames[gif->frameCount].lct.table[ii].red = rawGlobalColourTable[3*ii];
            gif->frames[gif->frameCount].lct.table[ii].green = rawGlobalColourTable[3*ii + 1];
            gif->frames[gif->frameCount].lct.table[ii].blue = rawGlobalColourTable[3*ii + 2];
        }
    }
    else {
        gif->frames[gif->frameCount].lct.table = NULL;
    }

    BYTE minimumCodeSize;
    fread(&minimumCodeSize, 1, 1, file);

    int codeSize = minimumCodeSize + 1;
    int clearCode = (1 << minimumCodeSize);

    decodeLZW(gif, file, codeSize, clearCode);
    gif->frameCount++;
}

Gif processGif(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        printf("File not opened\n");
        exit(1);
    }

    fseek(file, 6, SEEK_SET);

    Gif gif;
    gif.frames = (Frame*)malloc(sizeof(Frame));
    gif.frameCount = 0;

    fread(&gif.width, 2, 1, file);
    fread(&gif.height, 2, 1, file);

    fread(&gif.lsd, 1, 1, file);

    fread(&gif.gct.backgroundColour, 1, 1, file);

    unsigned short aspectRatio;
    fread(&aspectRatio, 1, 1, file);

    gif.gct.size = 3L * (1L << ((gif.lsd & 0x7) + 1));

    BYTE* rawGlobalColourTable = (BYTE*)malloc(sizeof(BYTE) * gif.gct.size);
    fread(rawGlobalColourTable, gif.gct.size, 1, file);

    gif.gct.table = (RGBColour*)malloc(sizeof(RGBColour) * (gif.gct.size / 3));
    for (int ii = 0; ii < gif.gct.size / 3; ii += 1) {
        gif.gct.table[ii].red = rawGlobalColourTable[3*ii];
        gif.gct.table[ii].green = rawGlobalColourTable[3*ii + 1];
        gif.gct.table[ii].blue = rawGlobalColourTable[3*ii + 2];
    }

    free(rawGlobalColourTable);

    BYTE seperator;
    fread(&seperator, 1, 1, file);
    while (seperator != 0x3B) {
        switch (seperator) {
            case 0x21: 
                processExtension(&gif, file);
                break;
            case 0x2C:
                processFrame(&gif, file);
                break;
        }

        fread(&seperator, 1, 1, file);
    }

    fclose(file);

    return gif;
}

void freeGif(Gif* gif) {
    free(gif->gct.table);
    for (int ii = 0; ii < gif->frameCount; ii++) {
        free(gif->frames[ii].lct.table);
    }
    free(gif->frames);
}