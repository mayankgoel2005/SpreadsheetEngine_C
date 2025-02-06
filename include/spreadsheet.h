#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#include "cell.h"

typedef struct {
    int rows;   // Total rows in the spreadsheet
    int cols;   // Total columns in the spreadsheet
    Cell **table; // 2D array of Cell structs
    int startRow; // Starting row for the visible portion
    int startCol; // Starting column for the visible portion
} Spreadsheet;

Spreadsheet *initializeSpreadsheet(int rows, int cols);
void printSpreadsheet(Spreadsheet *spreadsheet);
void freeSpreadsheet(Spreadsheet *spreadsheet);

#endif
