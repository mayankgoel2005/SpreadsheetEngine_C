#include <stdio.h>
#include "scrolling.h"

// Scroll up by 10 rows
void scrollUp(Spreadsheet *spreadsheet) {
    if (spreadsheet->startRow > 0) {
        spreadsheet->startRow -= 10;
        if (spreadsheet->startRow < 0) {
            spreadsheet->startRow = 0;
        }
    }
    printf("Scrolled up.\n");
}

// Scroll down by 10 rows
void scrollDown(Spreadsheet *spreadsheet) {
    if (spreadsheet->startRow + 10 < spreadsheet->rows) {
        spreadsheet->startRow += 10;
        if (spreadsheet->startRow + 10 > spreadsheet->rows) {
            spreadsheet->startRow = spreadsheet->rows - 10;
        }
    }
    printf("Scrolled down.\n");
}

// Scroll left by 10 columns
void scrollLeft(Spreadsheet *spreadsheet) {
    if (spreadsheet->startCol > 0) {
        spreadsheet->startCol -= 10;
        if (spreadsheet->startCol < 0) {
            spreadsheet->startCol = 0;
        }
    }
    printf("Scrolled left.\n");
}

// Scroll right by 10 columns
void scrollRight(Spreadsheet *spreadsheet) {
    if (spreadsheet->startCol + 10 < spreadsheet->cols) {
        spreadsheet->startCol += 10;
        if (spreadsheet->startCol + 10 > spreadsheet->cols) {
            spreadsheet->startCol = spreadsheet->cols - 10;
        }
    }
    printf("Scrolled right.\n");
}

void scrollTo(Spreadsheet *spreadsheet, int row, int col) {
    spreadsheet->startRow = row-1;
    spreadsheet->startCol = col-1;
}