#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "cell.h"
#include "avl_tree.h"  

void initCell(Cell *cell,int selfrow,int selfcol) {
    cell->value = 0;
    cell->op = OP_NONE;
    cell->row1 = cell->col1 = cell->row2 = cell->col2 = -1;
    cell->dependencies = NULL;
    cell->dependents = NULL;
    cell->operand1IsLiteral = 0;
    cell->operand1Literal = 0;
    cell->operand1 = NULL;
    cell->operand2IsLiteral = 0;
    cell->operand2Literal = 0;
    cell->operand2 = NULL;
    cell->selfRow=selfrow;
    cell->selfCol=selfcol;
    cell->error=0;
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
