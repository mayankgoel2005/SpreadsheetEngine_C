// spreadsheet.h
#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#include "cell.h"

typedef struct Spreadsheet {
    int rows, cols;
    int startRow, startCol; // For scrolling purposes.
    Cell **table;
} Spreadsheet;

Spreadsheet *initializeSpreadsheet(int rows, int cols);
void getColumnLabel(int colIndex, char *label);
void printSpreadsheet(Spreadsheet *spreadsheet);
void freeSpreadsheet(Spreadsheet *spreadsheet);

#endif // SPREADSHEET_H