#ifndef CELL_H
#define CELL_H

#include "avl_tree.h"  // If needed, include any dependencies

typedef struct Cell {
    int value;
    int op;  // 0 means direct assignment; other values represent formulas
    int row1, col1, row2, col2; // For advanced operations (range boundaries)
    
    // Pointers for dependency management (using AVLNode pointers)
    AVLNode *dependencies;
    AVLNode *dependents;
    
    // --- Fields for supporting mixed operands in simple operations ---
    int operand1IsLiteral;   // 1 if operand1 is literal; 0 if it’s a cell reference.
    int operand1Literal;     // Literal value for operand1 if operand1IsLiteral is 1.
    struct Cell *operand1;   // Pointer to the cell for operand1 if not a literal.
    
    int operand2IsLiteral;   // 1 if operand2 is literal.
    int operand2Literal;     
    struct Cell *operand2;
    
} Cell;

// Prototypes for functions related to a Cell.
void initCell(Cell *cell);
void freeCell(Cell *cell);

#endif // CELL_H
