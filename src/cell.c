// cell.c
#include <stdlib.h>
#include "cell.h"

// Initialize a cell: set formula fields to default values and dependency pointers to NULL.
void initCell(Cell *cell) {
    cell->op = 0;
    cell->row1 = -1;
    cell->col1 = -1;
    cell->row2 = -1;
    cell->col2 = -1;
    cell->value = 0;
    cell->isLiteral1 = 0;
    cell->isLiteral2 = 0;
    cell->literal1 = 0;
    cell->literal2 = 0;
    cell->cell1 = NULL;
    cell->cell2 = NULL;
    // For dependency tracking (AVL tree):
    cell->avlroot = NULL;
    cell->left = NULL;
    cell->right = NULL;
    cell->height = 0;
}

// Free resources used by a cell.
// (If your AVL tree nodes were allocated separately, you’d want to free them here.  
// In this simple example, we assume that the dependency AVL tree is freed as part of the spreadsheet cleanup.)
void freeCell(Cell *cell) {
    // For now, just set the pointer to NULL.
    cell->avlroot = NULL;
}