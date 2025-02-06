#include <stdio.h>
#include "scrolling.h"

// Scroll up by 5 rows
void scrollUp(Spreadsheet *spreadsheet) {
    if (spreadsheet->startRow > 0) {
        spreadsheet->startRow -= 5;
        if (spreadsheet->startRow < 0) {
            spreadsheet->startRow = 0;
        }
    }
    printf("Scrolled up.\n");
}

// Scroll down by 5 rows
void scrollDown(Spreadsheet *spreadsheet) {
    if (spreadsheet->startRow + 10 < spreadsheet->rows) {
        spreadsheet->startRow += 5;
        if (spreadsheet->startRow + 10 > spreadsheet->rows) {
            spreadsheet->startRow = spreadsheet->rows - 10;
        }
    }
    printf("Scrolled down.\n");
}

// Scroll left by 5 columns
void scrollLeft(Spreadsheet *spreadsheet) {
    if (spreadsheet->startCol > 0) {
        spreadsheet->startCol -= 5;
        if (spreadsheet->startCol < 0) {
            spreadsheet->startCol = 0;
        }
    }
    printf("Scrolled left.\n");
}

// Scroll right by 5 columns
void scrollRight(Spreadsheet *spreadsheet) {
    if (spreadsheet->startCol + 10 < spreadsheet->cols) {
        spreadsheet->startCol += 5;
        if (spreadsheet->startCol + 10 > spreadsheet->cols) {
            spreadsheet->startCol = spreadsheet->cols - 10;
        }
    }
    printf("Scrolled right.\n");
}
