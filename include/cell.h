#ifndef CELL_H
#define CELL_H

#include "avl_tree.h"  // For AVLNode

typedef struct Cell {
    int value;      // Current value.
    int op;         // Operation code: 0 = direct assignment, 1-4 = simple binary ops,
                    // 5+ = advanced ops (SUM, MIN, etc.), 10 = SLEEP.
    int row1, col1, row2, col2; // Used by advanced operations (range boundaries).

    AVLNode *dependencies;   // Dependency tree (only cell references).
    AVLNode *dependents;     // Dependents tree.

    // For simple (binary) formulas, store both operands:
    int operand1IsLiteral;   // 1 if operand1 is a literal; 0 if it is a cell reference.
    int operand1Literal;     // Literal value if operand1IsLiteral is 1.
    struct Cell *operand1;   // Pointer to the operand cell if operand1IsLiteral is 0.

    int operand2IsLiteral;   // 1 if operand2 is a literal.
    int operand2Literal;     
    struct Cell *operand2;
} Cell;

void initCell(Cell *cell);
void freeCell(Cell *cell);

#endif // CELL_H
