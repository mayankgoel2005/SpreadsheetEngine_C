#ifndef SIMPLE_OPERATIONS_H
#define SIMPLE_OPERATIONS_H

#include "spreadsheet.h"

// Function to handle simple operations (direct assignment or formulas).
// When a cell is updated, all cells in its dependents AVL tree are recalculated recursively.
void handleOperation(const char *input, Spreadsheet *spreadsheet, clock_t start);

#endif
