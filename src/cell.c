#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>  
#include "cell.h"

// initialized a cell.
void initCell(Cell *cell) {
    cell->op = 0;
    cell->value = 0;
    // Initialize the formula operands / range boundaries to invalid values.
    cell->row1 = cell->col1 = cell->row2 = cell->col2 = -1;
    // Set dependency trees to NULL.
    cell->dependencies = NULL;
    cell->dependents = NULL;
}

// freed dynamically allocated memory for dependency trees.
void freeCell(Cell *cell) {
    if (cell->dependencies) {
        avl_free(cell->dependencies);
        cell->dependencies = NULL;
    }
    if (cell->dependents) {
        avl_free(cell->dependents);
        cell->dependents = NULL;
    }
}

// convert a reference to row and column indices.
void parseCellReference(const char *ref, int *row, int *col) {
    *col = 0;
    int i = 0;
    while (isalpha(ref[i])) {
        *col = *col * 26 + (toupper(ref[i]) - 'A' + 1);
        i++;
    }
    *col = *col - 1;  // Convert from 1-based to 0-based.
    *row = atoi(ref + i) - 1;  
}
