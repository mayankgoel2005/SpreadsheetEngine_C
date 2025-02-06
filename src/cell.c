#include <stdlib.h>
#include "cell.h"

// Initialize a cell
void initCell(Cell *cell) {
    cell->op = 0;
    cell->row1 = -1;
    cell->col1 = -1;
    cell->row2 = -1;
    cell->col2 = -1;
    cell->value = 0;
    cell->dependencies = NULL;
    cell->dep_count = 0;
    cell->dependents = NULL;
    cell->dependents_count = 0;
}

// Free memory used by a cell
void freeCell(Cell *cell) {
    if (cell->dependencies) {
        free(cell->dependencies);
        cell->dependencies = NULL;
    }
    if (cell->dependents) {
        free(cell->dependents);
        cell->dependents = NULL;
    }
    cell->dep_count = 0;
    cell->dependents_count = 0;
}
