#ifndef CELL_H
#define CELL_H

#include "avl_tree.h"

typedef struct Cell {
    int row, col;         // Cell position (zero-based)
    int value;            // Current cell value
    int op;               // Operation code:
                          // 0 = constant,
                          // 1 = addition, 2 = subtraction, 3 = multiplication, 4 = division,
                          // 5 = SUM, 6 = MIN, 7 = MAX, 8 = AVG, 9 = STDEV (advanced operations)
    // For simple formulas, these hold the two operand cell references.
    // For advanced formulas, we use them to store the range boundaries (top-left and bottom-right).
    int row1, col1, row2, col2;

    // Dependency AVL trees (using your AVL tree implementation)
    AVLNode *dependencies; // AVL tree of pointers to cells this cell depends on.
    AVLNode *dependents;   // AVL tree of pointers to cells that depend on this cell.
} Cell;

void initCell(Cell *cell);
void freeCell(Cell *cell);
void parseCellReference(const char *ref, int *row, int *col);

#endif // CELL_H
