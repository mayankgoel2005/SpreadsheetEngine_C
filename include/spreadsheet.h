#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#include "cell.h"

typedef struct Spreadsheet {
    int display;
    int rows;
    int cols;
    int startRow;
    int startCol;
    Cell **table;
    // Global list for advanced (range) formulas.
    Cell **advancedFormulas;
    int advancedFormulasCount;
    int advancedFormulasCapacity;
} Spreadsheet;

Spreadsheet *initializeSpreadsheet(int rows, int cols);
void printSpreadsheet(Spreadsheet *spreadsheet);
void freeSpreadsheet(Spreadsheet *spreadsheet);
void handleOperation(const char *input, Spreadsheet *spreadsheet);

#endif  // SPREADSHEET_H
