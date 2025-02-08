#include <stdlib.h>
#include "cell.h"

void initCell(Cell *cell) {
    cell->op = 0;
    cell->value = 0;
    cell->dependencies = NULL;
    cell->dependents = NULL;
}

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
