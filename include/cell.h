#ifndef CELL_H
#define CELL_H

#include "avl_tree.h"

// Each cell now stores two AVL trees:
// - dependencies: AVL tree of cells that this cell depends on (its sources)
// - dependents: AVL tree of cells that depend on this cell
typedef struct Cell {
    int op;                // Operation code: 0 = direct value; 1 = addition; 2 = subtraction; 3 = multiplication; 4 = division.
    int value;             // Current value of the cell.
    AVLNode *dependencies; // AVL tree root storing pointers to source cells.
    AVLNode *dependents;   // AVL tree root storing pointers to cells that depend on this cell.
} Cell;

void initCell(Cell *cell);
void freeCell(Cell *cell);

#endif
